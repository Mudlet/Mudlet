/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QImage>
#include <QPointer>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSettings>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QWidget>
#include <QXmlStreamReader>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
// After a QtNetwork header, which is what defines QT_NO_SSL
#if !defined(QT_NO_SSL)
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#endif
#include <QtTest/QtTest>

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>

#include "CredentialManager.h"
#include "EventLoopPump.h"
#include "FontManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletMedia.h"
#include "PortableModeTestHelper.h"
#include "SpeechRecognizer.h"
#include "SpeechRecognizerFactory.h"
#include "TAppFrontend.h"
#include "TConsoleFrontend.h"
#include "TConsoleModel.h"
#include "TLabel.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMxpFrameFrontend.h"
#include "TNullAppFrontend.h"
#include "TNullConsoleFrontend.h"
#include "TNullMapViews.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TSpeechBridge.h"
#include "TriggerMatchPool.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "discord.h"

#include "GroupedTest.h"

// A stand-in engine, so a session can be started and ended with no speech library installed
class HeadlessStandInRecognizer : public SpeechRecognizer
{
    Q_OBJECT

public:
    explicit HeadlessStandInRecognizer(QObject* parent = nullptr)
    : SpeechRecognizer(parent)
    {
    }

    bool initialize(const QString& modelPath) override
    {
        if (mFailNextLoad) {
            mFailNextLoad = false;
            // As a model that turns out unusable once read: what it can do is announced before it fails
            mCapabilities.wordResults = !mCapabilities.wordResults;
            emit capabilitiesChanged(mCapabilities);
            setState(State::Error);
            emit errorOccurred(qsl("the stand-in could not load its model"));
            return false;
        }
        mModelPath = modelPath;
        setState(State::Ready);
        return true;
    }
    void failNextLoad() { mFailNextLoad = true; }
    QString currentLanguage() const override { return qsl("en"); }
    bool setLanguage(const QString&) override { return true; }
    QString backendName() const override { return qsl("HeadlessStandIn"); }
    QString backendVersion() const override { return qsl("1.0"); }
    Sensitivity sensitivity() const override { return Sensitivity::Default; }
    QString modelPath() const override { return mModelPath; }
    Capabilities capabilities() const override { return mCapabilities; }
    // Changed without saying so, as a library being swapped underneath an engine does
    void setBiasing(const bool biasing) { mCapabilities.biasing = biasing; }
    // A fault the engine reports itself, as sherpa's model rebuild can
    bool setSensitivity(Sensitivity) override
    {
        emit errorOccurred(qsl("the stand-in could not change its sensitivity"));
        return false;
    }

protected:
    void doStartListening() override { setState(State::Listening); }
    void doStopListening() override { setState(State::Ready); }
    void doCancel() override { setState(State::Ready); }

private:
    QString mModelPath;
    bool mFailNextLoad = false;
    // Biasing with no vocabulary of its own, so stt.setVocabulary() makes the base class report a fault
    Capabilities mCapabilities{.biasing = true, .sensitivityTuning = true};
};

// A profile made without ever starting the main window, so it only has the null
// console view: its Lua, triggers and main console model have to work regardless.
class HeadlessHostSmokeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the profile pool in the app; here nothing else would.
    std::unique_ptr<HostManager> mpHostManager;
    const QString mHostname = qsl("Test-Headless-Host-Smoke");
    // Sorted last, as the pool closes profiles in name order and a later one's save would also write its window,
    // masking a missing save of its own
    const QString mAppCloseHostname = qsl("Test-Headless-Host-Zz-Window-Layout-App-Close");
    const QString mConnectingHostname = qsl("Test-Headless-Host-Connect");
    const QString mSecureHostname = qsl("Test-Headless-Host-Secure");
    const QString mHeldLineHostname = qsl("Test-Headless-Host-Held-Line");
    const QString mWindowsHostname = qsl("Test-Headless-Host-Windows");
    // What initTestCase() writes to Mudlet.ini when MUDLET_TEST_MUDLET_INI is "present", each unlike its
    // default. Five threads give a worker count unlike min(4, cores / 2)'s on any machine of two cores or more.
    static constexpr int scmAutosaveMinutes = 7;
    static constexpr int scmUsernameDelayMs = 173;
    static constexpr int scmPasswordDelayMs = 241;
    static constexpr int scmMatchThreads = 5;
    static constexpr int scmMatchThreshold = 1000;

    static bool mudletIniWritten() { return qgetenv("MUDLET_TEST_MUDLET_INI") == "present"; }

    bool mAppCloseLayoutChecked = false;

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static bool mainBufferHolds(Host* host, const QString& text) { return host->mainConsoleModel().buffer.lineBuffer.join(QChar::LineFeed).contains(text); }

    // The shape a game serves for an MMP map download
    static QByteArray xmlMap()
    {
        return QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<map>
 <areas>
  <area id="1" name="Headless Xml Area"/>
 </areas>
 <rooms>
  <room id="3" area="1" title="Headless Xml Room" environment="2">
   <coord x="0" y="0" z="0"/>
  </room>
 </rooms>
</map>
)");
    }

    static QString headlessLayoutPath() { return MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat")); }
    static bool writeHeadlessLayout(const QByteArray& bytes)
    {
        QFile layoutFile(headlessLayoutPath());
        return layoutFile.open(QIODevice::WriteOnly) && layoutFile.write(bytes) == bytes.size();
    }
    // The headless layout file's magic and a later version, which a save refuses to replace
    static QByteArray laterVersionLayout() { return QByteArray::fromHex("4d574c4800000002"); }
    static void ignoreLaterVersionLayout()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("has version 2 not 1 - none of it will be loaded, nor a layout saved over it, until it is deleted or renamed$")));
    }
    static void ignoreRefusedLayoutSave() { QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("has version 2 not 1 - this save does not replace it: delete or rename it to save one$"))); }
    static QMap<QString, std::pair<QRect, bool>> savedUserWindows()
    {
        QMap<QString, std::pair<QRect, bool>> placements;
        QFile layoutFile(headlessLayoutPath());
        if (layoutFile.open(QIODevice::ReadOnly)) {
            QDataStream ifs(&layoutFile);
            ifs.setVersion(QDataStream::Qt_5_12);
            quint32 magic = 0;
            quint32 version = 0;
            ifs >> magic >> version >> placements;
        }
        return placements;
    }

    static QString veraFontFolder() { return qsl("%1/ttf-bitstream-vera-1.10").arg(MudletApp::getMudletPath(enums::mainFontsPath)); }

    // A second of silence, for media that only has to be played and stopped
    static QByteArray silentWav()
    {
        constexpr quint32 sampleRate = 8000;
        constexpr quint32 dataBytes = sampleRate * 2;
        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << static_cast<quint32>(36 + dataBytes);
        out.writeRawData("WAVEfmt ", 8);
        out << static_cast<quint32>(16) << static_cast<quint16>(1) << static_cast<quint16>(1) << sampleRate << sampleRate * 2 << static_cast<quint16>(2) << static_cast<quint16>(16);
        out.writeRawData("data", 4);
        out << dataBytes;
        wav.append(QByteArray(dataBytes, '\0'));
        return wav;
    }

    // A binary map of the profile's own, holding just room roomId; the profile's map is left empty
    static QByteArray binaryMapOf(Host* host, const int roomId)
    {
        TMap* map = host->mpMap.data();
        map->mapClear();
        const int areaId = map->mpRoomDB->addArea(qsl("Headless Binary Area"));
        map->addRoom(roomId);
        map->setRoomArea(roomId, areaId);
        QByteArray serialized;
        QDataStream out(&serialized, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        map->serialize(out);
        map->mapClear();
        return serialized;
    }

    // A saved profile whose script appends "<profile> <what>" to recordPath for its start, its sysLoadEvent and
    // its sysExitEvent, and defines headlessRecord(what) for more
    static bool writeRecordingProfile(const QString& name, const bool saveOnExit, const QString& recordPath)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        // Opened binary so Windows writes no \r
        const QString script = qsl(R"lua(function headlessRecord(what)
  local file = io.open([[%1]], "ab")
  file:write(getProfileName(), " ", what, "\n")
  file:close()
end
headlessRecord("started")
registerAnonymousEventHandler("sysLoadEvent", function() headlessRecord("sysLoadEvent") end)
registerAnonymousEventHandler("sysExitEvent", function() headlessRecord("sysExitEvent") end))lua")
                                       .arg(recordPath);
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host mFORCE_SAVE_ON_EXIT="%1"></Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessRecording</name><packageName></packageName><script>%2</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(saveOnExit ? qsl("yes") : qsl("no"), script.toHtmlEscaped())
                               .toUtf8());
        return true;
    }

    static QString readRecord(const QString& recordPath)
    {
        QFile file(recordPath);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    static qsizetype saveCount(const QString& name) { return QDir(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)).entryList(QStringList{qsl("*.xml")}, QDir::Files).size(); }

    // Whether the profile's newest save, not the one writeRecordingProfile() made, reads to its end
    static bool newestSaveIsComplete(const QString& name)
    {
        const QDir folder(MudletApp::getMudletPath(enums::profileXmlFilesPath, name));
        const QStringList saves = folder.entryList(QStringList{qsl("*.xml")}, QDir::Files, QDir::Name | QDir::Reversed);
        if (saves.isEmpty() || saves.first() == qsl("2020-01-01#00-00-00.xml")) {
            return false;
        }
        QFile file(folder.filePath(saves.first()));
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        QXmlStreamReader reader(&file);
        bool sawHost = false;
        while (!reader.atEnd()) {
            reader.readNext();
            sawHost |= reader.isStartElement() && reader.name() == qsl("Host");
        }
        return sawHost && !reader.hasError();
    }

    // Stands in for any Lua API that runs the event loop, which the profile has to outlast
    static void registerRunEventLoop(Host* pHost)
    {
        // One for each profile, which its C function reaches through an upvalue
        static std::vector<std::unique_ptr<QPointer<Host>>> sRunning;
        sRunning.push_back(std::make_unique<QPointer<Host>>(pHost));
        lua_State* state = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_pushlightuserdata(state, sRunning.back().get());
        lua_pushcclosure(
                state,
                [](lua_State* L) -> int {
                    const auto* pRunning = static_cast<const QPointer<Host>*>(lua_touserdata(L, lua_upvalueindex(1)));
                    QTest::qWait(200);
                    if (!*pRunning) {
                        // Returning would run on its closed Lua state
                        qFatal("The profile was deleted under its own script as it ran the event loop.");
                    }
                    return 0;
                },
                1);
        lua_setglobal(state, "runEventLoop");
    }

    // So that whichever map save is found afterwards is a new one
    static bool clearMapSaves(const QString& name)
    {
        QDir folder(MudletApp::getMudletPath(enums::profileMapsPath, name));
        const QStringList saves = folder.entryList(QStringList{qsl("*.dat")}, QDir::Files);
        for (const QString& save : saves) {
            if (!folder.remove(save)) {
                return false;
            }
        }
        return true;
    }

    // Read into a profile of a pool of its own, so the pool under test is left as it was
    static bool newestMapSaveHoldsRoom(const QString& name, const int roomId)
    {
        const QDir folder(MudletApp::getMudletPath(enums::profileMapsPath, name));
        const QStringList saves = folder.entryList(QStringList{qsl("????-??-??#??-??-??map.dat")}, QDir::Files, QDir::Name | QDir::Reversed);
        if (saves.isEmpty()) {
            return false;
        }
        HostManager pool;
        const QString readerName = qsl("Test-Headless-Host-Map-Reader");
        if (!pool.addHost(readerName, QString(), QString(), QString())) {
            return false;
        }
        TMap* map = pool.getHost(readerName)->mpMap.data();
        return map->restore(folder.filePath(saves.first())) && map->mpRoomDB->getRoom(roomId);
    }

    // What QCoreApplication::exec() returns once closeMudlet() quits, or 2 if it has not by the timeout
    int execUntilQuit(const std::chrono::milliseconds timeout)
    {
        QTimer watchdog;
        watchdog.setSingleShot(true);
        connect(&watchdog, &QTimer::timeout, this, []() {
            QCoreApplication::exit(2);
        });
        watchdog.start(timeout);
        return QCoreApplication::exec();
    }

    // As the stt.* bindings count one, the built-in macOS engine included
    static bool speechEngineAvailable()
    {
        return SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Auto) || SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform);
    }

    static HeadlessStandInRecognizer* standInEngine() { return TSpeechBridge::instance() ? qobject_cast<HeadlessStandInRecognizer*>(TSpeechBridge::instance()->speechRecognizer()) : nullptr; }

    // Named, as the bridge keeps an engine it has for Auto and for its own backend; loaded, as with no
    // engine library on macOS each stt.start() builds another (see SpeechAcrossProfilesTest)
    static bool installHeadlessStandInEngine()
    {
        if (!TSpeechBridge::instance()) {
            return false;
        }
        SpeechRecognizerFactory::setFactoryOverride([](QObject* parent) -> SpeechRecognizer* {
            auto* engine = new HeadlessStandInRecognizer(parent);
            engine->initialize(QString());
            return engine;
        });
        for (const auto backend : {SpeechRecognizerFactory::Backend::Sherpa, SpeechRecognizerFactory::Backend::Vosk}) {
            TSpeechBridge::instance()->initSpeechRecognition(backend);
            if (standInEngine()) {
                return true;
            }
        }
        return false;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        // Before the pool exists, as a player's own Mudlet.ini would be, and only for the run of its own that
        // CMakeLists.txt makes: the trigger match pool reads it once, and every other test here wants the defaults
        if (mudletIniWritten()) {
            QFile ini(qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()));
            QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
            ini.write(qsl("[General]\n"
                          "autosaveIntervalMinutes=%1\n"
                          "mapCenterSmallAreas=true\n"
                          "autoLoginUsernameDelay=%2\n"
                          "autoLoginPasswordDelay=%3\n"
                          "triggerMatchThreads=%4\n"
                          "triggerMatchThreshold=%5\n"
                          "enableMuteAPI=true\n"
                          "enableMuteGame=true\n"
                          "reportMapIssuesToConsole=true\n"
                          "storePasswordsSecurely=false\n")
                              .arg(scmAutosaveMinutes)
                              .arg(scmUsernameDelayMs)
                              .arg(scmPasswordDelayMs)
                              .arg(scmMatchThreads)
                              .arg(scmMatchThreshold)
                              .toUtf8());
            ini.close();
        }
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
#ifdef INCLUDE_FONTS
        // Staged before anything can ask for a font: one bundled font copied out, as a run of the executable
        // can leave a bundle, so the rest of that bundle has to come from the resources
        QVERIFY(QDir().mkpath(veraFontFolder()));
        QVERIFY(QFile::copy(qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf"), qsl("%1/Vera.ttf").arg(veraFontFolder())));
#endif

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
        // Skipped as a whole, so a failure in one of its tests cannot hide behind the skip
        if (qgetenv("MUDLET_TEST_DISCORD_LIBRARY") == "present" && !Discord::self()->libraryLoaded()) {
            QSKIP("The bundled Discord library cannot be loaded here");
        }
    }

    void cleanupTestCase()
    {
        const auto restoreXdg = qScopeGuard([this]() {
            mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        });
        if (!mpHostManager) {
            return;
        }
        QFile layoutFile(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat")));
        // initTestCase() may have skipped before redirecting the config dir to this test's own
        const bool ownLayoutFile = layoutFile.fileName().startsWith(mConfigDir.path() + QLatin1Char('/'));
        if (ownLayoutFile) {
            layoutFile.remove();
        }
        // A pool outliving the one self() answers with, whose profiles close after it with no pool to save them: what
        // they leave must not be held over for a later pool's save
        const QString outlivingHostname = qsl("Test-Headless-Host-Window-Layout-Outliving-Pool");
        std::unique_ptr<HostManager> pOutliving;
        if (ownLayoutFile) {
            QTest::ignoreMessage(QtWarningMsg, "HostManager::HostManager() WARNING - a HostManager already exists, so self() keeps pointing at that one.");
            pOutliving = std::make_unique<HostManager>();
            if (pOutliving->addHost(outlivingHostname, QString(), QString(), QString())) {
                pOutliving->getHost(outlivingHostname)->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 250, 160)"));
            }
        }
        // Read before the pool goes, and compared after, so a mismatch still leaves the environment as it found it
        const QString lastHostname = mAppCloseLayoutChecked ? HostManager::self()->hostList().constLast()->getName() : QString();
        // Mudlet.ini is read by the first pool alone, so a pool made once it has gone starts from its mutes:
        // the one as read, the other as changed since
        const bool apiMuted = mudletIniWritten();
        const bool gameMuted = !mudletIniWritten();
        QCOMPARE(mpHostManager->media().apiMuted(), apiMuted);
        mpHostManager->media().setGameMuted(gameMuted);
        mpHostManager.reset();
        // ~HostManager() saves as each profile closes, after its sysExitEvent, as TMainConsole::closeEvent() asks for one in the GUI
        const bool appCloseSaved = QFile::exists(layoutFile.fileName());
        quint32 magic = 0;
        quint32 version = 0;
        QMap<QString, std::pair<QRect, bool>> placements;
        if (ownLayoutFile && mAppCloseLayoutChecked && layoutFile.open(QIODevice::ReadOnly)) {
            QDataStream ifs(&layoutFile);
            ifs.setVersion(QDataStream::Qt_5_12);
            ifs >> magic >> version >> placements;
            layoutFile.close();
        }
        bool outlivingHeld = false;
        bool nextPoolSaved = true;
        if (pOutliving) {
            QTest::ignoreMessage(QtWarningMsg,
                                 QRegularExpression(qsl("with no HostManager, the window layout could not be saved, so where these were is lost: QList\\(\"%1/wlA\"\\)$").arg(outlivingHostname)));
            pOutliving->deleteHost(outlivingHostname);
            pOutliving.reset();
            layoutFile.remove();
            const HostManager nextPool;
            nextPoolSaved = TAppFrontend::instance()->saveWindowLayoutForScript();
            outlivingHeld = savedUserWindows().contains(outlivingHostname + qsl("/wlA"));
        }
        if (ownLayoutFile) {
            layoutFile.remove();
        }
        QVERIFY2(nextPoolSaved, "A later pool could not save, so what it held of an outliving pool's profile went unchecked.");
        QVERIFY2(!outlivingHeld, "A pool outliving the one self() answers with left its profile's windows for a later pool to save.");
        if (mAppCloseLayoutChecked) {
            QCOMPARE(lastHostname, mAppCloseHostname);
        }
        if (ownLayoutFile && mAppCloseLayoutChecked) {
            QVERIFY2(appCloseSaved, "Closing the app saved no layout.");
            QCOMPARE(magic, quint32(0x4d574c48));
            QCOMPARE(version, quint32(1));
            const auto placement = placements.value(mAppCloseHostname + qsl("/wlA"));
            QCOMPARE(placement.first, QRect(9, 10, 132, 62));
            QVERIFY(placement.second);
        }
        HostManager nextPool;
        QCOMPARE(nextPool.media().apiMuted(), apiMuted);
        QCOMPARE(nextPool.media().gameMuted(), gameMuted);
    }

    void test_profileRunsLuaAndTriggersWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QVERIFY2(HostManager::self()->addHost(mHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->consoleFrontend(), "consoleFrontend() must never be null.");

        // pcall keeps the failing assert's message, which compileAndExecuteScript() only logs
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessResult = 'not run'
local ok, err = pcall(function()
  assert(echo("main", "headless echo line\n") == true, "echo to main did not answer true")
  local missingOk, missingMsg = echo("noSuchHeadlessWindow", "text")
  assert(missingOk == nil and type(missingMsg) == "string", "echo to a missing window did not answer nil and a message")
  headlessTriggerHit = 'none'
  local id = tempTrigger("headless fed line", [[headlessTriggerHit = line; echo("main", "headless trigger echo\n")]])
  assert(id, "tempTrigger made no trigger")
  assert(feedTriggers("headless fed line\n") == true, "feedTriggers did not answer true")
end)
headlessResult = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "headlessTriggerHit"), qsl("headless fed line"));
        QVERIFY2(mainBufferHolds(host, qsl("headless echo line")), "echo() to main never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("headless fed line")), "feedTriggers() never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("headless trigger echo")), "The trigger's echo never reached the main console model.");
        // Lets work the profile deferred, such as its first-launch timer, run before looking
        QCoreApplication::processEvents();
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making and running the profile created a widget.");
    }

    void test_windowsMadeWithNoMainWindowHaveModels()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QVERIFY2(HostManager::self()->addHost(mWindowsHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
        host->registerAnonymousEventHandler(qsl("sysLabelDeleted"), qsl("onHeadlessDeleted"));
        host->registerAnonymousEventHandler(qsl("sysMiniConsoleDeleted"), qsl("onHeadlessDeleted"));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessWindows = 'not run'
headlessDeleted = {}
function onHeadlessDeleted(_, name) headlessDeleted[#headlessDeleted + 1] = name end
local ok, err = pcall(function()
  assert(createMiniConsole("headlessMini", 10, 20, 300, 200) == true, "createMiniConsole did not answer true")
  assert(windowType("headlessMini") == "miniconsole", "the miniconsole has no type")
  assert(echo("headlessMini", "mini line\n") == true, "echo to the miniconsole did not answer true")
  local again, againMsg = createMiniConsole("headlessMini", 0, 0, 10, 10)
  assert(again == false and againMsg:find("already exists"), "a second miniconsole of that name was not refused")

  assert(openUserWindow("headlessWindow") == true, "openUserWindow did not answer true")
  assert(windowType("headlessWindow") == "userwindow", "the user window has no type")
  assert(echo("headlessWindow", "window line\n") == true, "echo to the user window did not answer true")
  local placed, placedMsg = openUserWindow("headlessWindow", false, false, "sideways")
  assert(placed == nil and placedMsg:find("docking option"), "an unknown docking area was not refused")
  assert(windowType("headlessWindow") == "userwindow", "refusing the area took the user window away")
  assert(createMiniConsole("headlessWindow", "headlessNested", 0, 0, 50, 50) == true, "a miniconsole could not go in the user window")
  assert(createLabel("headlessWindow", "headlessNestedLabel", 0, 0, 50, 50, 1) == true, "a label could not go in the user window")
  local orphan, orphanMsg = createMiniConsole("noSuchHeadlessWindow", "headlessOrphan", 0, 0, 50, 50)
  assert(orphan == false and orphanMsg:find("not found"), "a miniconsole went into a window that does not exist")

  assert(createLabel("headlessLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(windowType("headlessLabel") == "label", "the label has no type")
  assert(echo("headlessLabel", "label text") == true, "echo to the label did not answer true")
  local clash, clashMsg = createLabel("headlessMini", 0, 0, 10, 10, 1)
  assert(clash == false and clashMsg:find("already exists"), "a label took the name of a miniconsole")

  createBuffer("headlessBuffer")
  assert(windowType("headlessBuffer") == "buffer", "the buffer has no type")
  assert(echo("headlessBuffer", "buffer line\n") == true, "echo to the buffer did not answer true")

  assert(deleteLabel("headlessLabel") == true, "deleteLabel did not answer true")
  assert(windowType("headlessLabel") == nil, "the deleted label still has a type")
  assert(echo("headlessLabel", "text") == nil, "echo to the deleted label did not fail")
  local gone, goneMsg = deleteLabel("headlessLabel")
  assert(gone == false and goneMsg:find("not found"), "deleting a missing label did not fail")

  assert(deleteMiniConsole("headlessMini") == true, "deleteMiniConsole did not answer true")
  assert(windowType("headlessMini") == nil, "the deleted miniconsole still has a type")
  assert(echo("headlessMini", "text") == nil, "echo to the deleted miniconsole did not fail")

  assert(deleteMiniConsole("headlessWindow") == true, "deleting the user window did not answer true")
  assert(windowType("headlessWindow") == nil, "the deleted user window still has a type")
  assert(windowType("headlessNested") == nil, "a miniconsole in the user window outlived it")
  assert(windowType("headlessNestedLabel") == nil, "a label in the user window outlived it")
end)
headlessWindows = ok and 'ok' or tostring(err)
headlessDeletedNames = table.concat(headlessDeleted, ",")
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessWindows"), qsl("ok"));
        // Only what a script deleted, as the GUI raises nothing for what goes with a user window
        QCOMPARE(luaGlobalString(host, "headlessDeletedNames"), qsl("headlessLabel,headlessMini,headlessWindow"));

        const TConsoleModel* buffer = host->windowRegistry().subConsoleModel(qsl("headlessBuffer"));
        QVERIFY2(buffer, "The buffer has no model.");
        QVERIFY(buffer->buffer.lineBuffer.join(QChar::LineFeed).contains(qsl("buffer line")));
        QVERIFY2(!mainBufferHolds(host, qsl("buffer line")), "Text echoed to the buffer reached the main console.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the windows created a widget.");
    }

    void test_labelTextReachesModelWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(createLabel("headlessTextLabel", 5, 6, 70, 80, 1); echo("headlessTextLabel", "label words"))lua")));
        const TLabelModel* label = host->windowRegistry().labelModel(qsl("headlessTextLabel"));
        QVERIFY2(label, "The label has no model.");
        QCOMPARE(label->mText, qsl("label words"));
        QCOMPARE(label->mGeometry, QRect(5, 6, 70, 80));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessLabelText = getLabelText("headlessTextLabel"))lua")));
        QCOMPARE(luaGlobalString(host, "headlessLabelText"), qsl("label words"));
    }

    // The answers are those UI_spec.lua's accessibility cases expect of a real label, and TLabel's for what it shows.
    void test_labelAccessibilityWithNoMainWindow()
    {
        const QString hostname = qsl("Test-Headless-Host-Label-Accessibility");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir imageDir;
        QVERIFY(imageDir.isValid());
        const auto writeFile = [&imageDir](const QString& name, const QByteArray& data) {
            QFile file(imageDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("shape.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"10\"><rect width=\"20\" height=\"10\"/></svg>")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("a11yDir = [==[%1]==]").arg(imageDir.path())));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelAccess = 'not run'
local ok, err = pcall(function()
  local still, gif, svg = a11yDir .. "/still.png", a11yDir .. "/anim.gif", a11yDir .. "/shape.svg"
  assert(createLabel("a11yLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")

  echo("a11yLabel", "&nbsp;&nbsp;<b>Map</b> &amp; Exits")
  assert(getLabelAccessibleName("a11yLabel") == "Map & Exits", "an unnamed label is not named after its text")
  assert(setLabelAccessibleName("a11yLabel", "Open the map") == true, "setLabelAccessibleName did not answer true")
  assert(getLabelAccessibleName("a11yLabel") == "Open the map", "the accessible name did not read back")
  assert(setLabelAccessibleName("a11yLabel", "") == true, "clearing the accessible name did not answer true")
  assert(getLabelAccessibleName("a11yLabel") == "Map & Exits", "a cleared name did not go back to the text")
  echo("a11yLabel", "")
  setLabelToolTip("a11yLabel", "Open the map")
  assert(getLabelAccessibleName("a11yLabel") == "Open the map", "a label with no text is not named after its tooltip")

  assert(getLabelAccessibleRole("a11yLabel") == "text", "a label with no callbacks is not text")
  setLabelReleaseCallback("a11yLabel", function() end)
  assert(getLabelAccessibleRole("a11yLabel") == "button", "a label with a release callback is not a button")
  setLabelReleaseCallback("a11yLabel", nil)
  setLabelClickCallback("a11yLabel", function() end)
  assert(getLabelAccessibleRole("a11yLabel") == "button", "a label with a click callback is not a button")
  assert(setLabelAccessibleRole("a11yLabel", "text") == true, "setLabelAccessibleRole did not answer true")
  assert(getLabelAccessibleRole("a11yLabel") == "text", "the role a script gave did not win over the callbacks")
  assert(setLabelAccessibleRole("a11yLabel", "") == true, "clearing the role did not answer true")
  assert(enableClickthrough("a11yLabel") == true, "enableClickthrough did not answer true")
  assert(getLabelAccessibleRole("a11yLabel") == "text", "a click-through label is a button")
  disableClickthrough("a11yLabel")
  assert(getLabelAccessibleRole("a11yLabel") == "button", "a label no longer click-through is not a button again")
  setLabelClickCallback("a11yLabel", nil)
  local badRole, badRoleMsg = setLabelAccessibleRole("a11yLabel", "slider")
  assert(badRole == nil and badRoleMsg:find("'slider' is not a valid role", 1, true), "an unknown role was not refused")

  assert(setBackgroundImage("a11yLabel", still) == true, "setBackgroundImage did not answer true")
  assert(getLabelAccessibleRole("a11yLabel") == "graphic", "a label showing a picture is not a graphic")
  assert(setMovie("a11yLabel", gif) == true, "setMovie did not answer true")
  assert(getLabelAccessibleRole("a11yLabel") == "animation", "a label showing a movie is not an animation")
  echo("a11yLabel", "words")
  assert(getLabelAccessibleRole("a11yLabel") == "text", "a label given text after a movie is not text")
  assert(setBackgroundImage("a11yLabel", svg) == true, "setBackgroundImage did not take the SVG")
  assert(getLabelAccessibleRole("a11yLabel") == "text", "text over an SVG is not what the label shows")
  echo("a11yLabel", "")
  assert(getLabelAccessibleRole("a11yLabel") == "graphic", "an SVG with no text over it is not a graphic")
  resetBackgroundImage("a11yLabel")
  assert(getLabelAccessibleRole("a11yLabel") == "text", "a label with its image reset is not text")

  assert(getLabelLiveRegion("a11yLabel") == "off", "the live region of a new label is not off")
  for _, mode in ipairs({"polite", "assertive", "off"}) do
    assert(setLabelLiveRegion("a11yLabel", mode) == true, "setLabelLiveRegion did not answer true for " .. mode)
    assert(getLabelLiveRegion("a11yLabel") == mode, "the live region did not read back as " .. mode)
  end
  local badMode, badModeMsg = setLabelLiveRegion("a11yLabel", "loud")
  assert(badMode == nil and badModeMsg:find("'loud' is not a valid live region", 1, true), "an unknown live region was not refused")

  local missing = "label name 'noSuchA11yLabel' not found"
  for name, call in pairs({
    getLabelAccessibleName = function() return getLabelAccessibleName("noSuchA11yLabel") end,
    setLabelAccessibleName = function() return setLabelAccessibleName("noSuchA11yLabel", "n") end,
    getLabelAccessibleRole = function() return getLabelAccessibleRole("noSuchA11yLabel") end,
    setLabelAccessibleRole = function() return setLabelAccessibleRole("noSuchA11yLabel", "text") end,
    getLabelLiveRegion = function() return getLabelLiveRegion("noSuchA11yLabel") end,
    setLabelLiveRegion = function() return setLabelLiveRegion("noSuchA11yLabel", "polite") end,
  }) do
    local value, message = call()
    assert(value == nil and message == missing, name .. " did not refuse a missing label as the GUI does: " .. tostring(message))
  end
end)
headlessLabelAccess = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelAccess"), qsl("ok"));
        const TLabelModel* label = host->windowRegistry().labelModel(qsl("a11yLabel"));
        QVERIFY2(label, "The label has no model.");
        QCOMPARE(label->mLiveRegion, TLabelLiveRegion::Off);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The accessibility calls created a widget.");
    }

    // What a real view records as it moves, shows, styles or titles a window, the null view records too.
    void test_windowStateReadsBackWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessState = 'not run'
local ok, err = pcall(function()
  assert(createLabel("stateLabel", 1, 2, 30, 40, 1) == true, "createLabel did not answer true")
  assert(getLabelStyleSheet("stateLabel") == "background-color: rgba(32, 32, 32, 255);", "the new label is not styled with its colour")
  setBackgroundColor("stateLabel", 1, 2, 3, 4)
  local lr, lg, lb, la = getBackgroundColor("stateLabel")
  assert(lr == 1 and lg == 2 and lb == 3 and la == 4, "the background colour of the label did not read back")
  assert(getLabelStyleSheet("stateLabel") == "background-color: rgba(1, 2, 3, 4);", "the label was not restyled with its colour")
  assert(setLabelStyleSheet("stateLabel", "color: red;") == true, "setLabelStyleSheet did not answer true")
  assert(getLabelStyleSheet("stateLabel") == "color: red;", "the style sheet did not read back")
  assert(setLabelToolTip("stateLabel", "a tip") == true, "setLabelToolTip did not answer true")
  assert(getLabelToolTip("stateLabel") == "a tip", "the tool tip did not read back")
  assert(setLabelCursor("stateLabel", 2) == true, "setLabelCursor did not answer true")
  local badShape, badShapeMsg = setLabelCursor("stateLabel", 99)
  assert(badShape == nil and badShapeMsg:find("cursor shape"), "an unknown cursor shape was not refused")
  local badName, badNameMsg = setLabelCursor("stateLabel", "NoSuchCursor")
  assert(badName == nil and badNameMsg:find("cursor shape 'NoSuchCursor' not found", 1, true) == 1 and badNameMsg:find("wiki.mudlet.org/w/CursorShapes", 1, true), "an unknown cursor name was not refused")
  local noLabel, noLabelMsg = setLabelCursor("noSuchStateLabel", "NoSuchCursor")
  assert(noLabel == nil and noLabelMsg == "label name 'noSuchStateLabel' not found", "a missing label was not refused before an unknown cursor name")
  local missing, missingMsg = setLabelStyleSheet("noSuchStateLabel", "color: red;")
  assert(missing == nil and missingMsg:find("not found"), "styling a missing label did not answer nil and a message")
  assert(setLabelClickCallback("stateLabel", function() end) == true, "setLabelClickCallback did not answer true")
  assert(windowVisible("stateLabel") == true, "the new label is not visible")
  hideWindow("stateLabel")
  assert(windowVisible("stateLabel") == false, "the hidden label is visible")
  assert(showWindow("stateLabel") == true, "showWindow did not answer true")
  assert(windowVisible("stateLabel") == true, "the shown label is not visible")
  moveWindow("stateLabel", 5, 6)
  resizeWindow("stateLabel", 70, 80)

  assert(createMiniConsole("stateMini", 0, 0, 10, 10) == true, "createMiniConsole did not answer true")
  moveWindow("stateMini", 7, 8)
  resizeWindow("stateMini", 90, 100)
  hideWindow("stateMini")
  assert(windowVisible("stateMini") == false, "the hidden miniconsole is visible")
  setBackgroundColor("stateMini", 10, 20, 30, 255)
  local r, g, b, a = getBackgroundColor("stateMini")
  assert(r == 10 and g == 20 and b == 30 and a == 255, "the background colour of the miniconsole did not read back")

  assert(openUserWindow("stateWindow") == true, "openUserWindow did not answer true")
  assert(createLabel("stateWindow", "stateInner", 0, 0, 10, 10, 1) == true, "a label could not go in the user window")
  hideWindow("stateWindow")
  assert(windowVisible("stateWindow") == false, "the hidden user window is visible")
  assert(windowVisible("stateInner") == false, "a label in a hidden user window is visible")
  showWindow("stateWindow")
  assert(windowVisible("stateInner") == true, "a label in a shown user window is not visible")
  assert(setWindow("main", "stateInner", 3, 4, false) == true, "setWindow did not move the label")
  hideWindow("stateWindow")
  assert(windowVisible("stateInner") == false, "a label moved out unshown is visible")
  showWindow("stateWindow")
  assert(setUserWindowTitle("stateWindow", "A title") == true, "setUserWindowTitle did not answer true")
  assert(getUserWindowTitle("stateWindow") == "A title", "the title did not read back")
  assert(setUserWindowTitle("stateWindow") == true, "resetting the title did not answer true")
  assert(getUserWindowTitle("stateWindow"):find("stateWindow", 1, true), "the default title does not name the window")
  local notWindow, notWindowMsg = setUserWindowTitle("stateMini", "A title")
  assert(notWindow == nil and notWindowMsg:find("not a user window"), "a miniconsole took a title")
  assert(setUserWindowStyleSheet("stateWindow", "background: blue;") == true, "setUserWindowStyleSheet did not answer true")
  assert(getUserWindowStyleSheet("stateWindow") == "background: blue;", "the user window style sheet did not read back")
  resizeWindow("stateWindow", 120, 130)

  setBorderColor(1, 2, 3)
  local br, bg, bb = getBorderColor()
  assert(br == 1 and bg == 2 and bb == 3, "the border colour did not read back")
end)
headlessState = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessState"), qsl("ok"));
        const TLabelModel* label = host->windowRegistry().labelModel(qsl("stateLabel"));
        QVERIFY2(label, "The label has no model.");
        QCOMPARE(label->mGeometry, QRect(5, 6, 70, 80));
        QVERIFY(label->mClickFunction);
        QCOMPARE(host->windowRegistry().subConsoleGeometry(qsl("stateMini")), std::optional<QRect>(QRect(7, 8, 90, 100)));
        QCOMPARE(host->windowRegistry().labelModel(qsl("stateInner"))->mGeometry.topLeft(), QPoint(3, 4));
        QCOMPARE(host->windowRegistry().userWindowSize(qsl("stateWindow")), std::optional<QSize>(QSize(120, 130)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Working the windows created a widget.");
    }

    // With nothing to lay the windows out, the main console is the character grid NAWS reports and every
    // other window's grid is its size in its own font, counted as TTextEdit counts a real one's.
    void test_windowsHaveCharacterGridGeometryWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessGrid = 'not run'
local ok, err = pcall(function()
  local function joined(...)
    return table.concat({...}, ",")
  end
  gridMainSize = joined(getMainWindowSize())
  gridMainGeometry = joined(getWindowGeometry("main"))
  gridMainCells = joined(getColumnCount("main"), getRowCount("main"))
  gridMainFont = joined(getFont("main"), getFontSize("main"))
  gridMainCharSize = joined(calcFontSize("main"))
  assert(joined(getUserWindowSize("main")) == gridMainSize, "the main window has a user window size of its own")

  gridFontEvents = {}
  function gridFontChanged(_, window, family, size)
    gridFontEvents[#gridFontEvents + 1] = joined(window, size)
  end
  registerAnonymousEventHandler("sysFontChangeEvent", "gridFontChanged")
  assert(createMiniConsole("gridMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  gridMiniCells = joined(getColumnCount("gridMini"), getRowCount("gridMini"), getFontSize("gridMini"))
  gridMiniCharSize = joined(calcFontSize("gridMini"))
  assert(setFontSize("gridMini", 15) == true, "setFontSize did not answer true for the miniconsole")
  assert(setFontSize("gridMini", 15) == true, "setFontSize did not answer true for an unchanged size")
  gridMiniBiggerCells = joined(getColumnCount("gridMini"), getRowCount("gridMini"), getFontSize("gridMini"))
  resizeWindow("gridMini", 600, 400)
  gridMiniResizedCells = joined(getColumnCount("gridMini"), getRowCount("gridMini"))
  gridMiniColumnSize = joined(calcFontSize("gridMini", true))
  local family
  for name in pairs(getAvailableFonts()) do
    if name ~= getFont("gridMini") and name ~= "Bitstream Vera Sans Mono" then
      family = name
      break
    end
  end
  assert(setFont("gridMini", family) == true, "setFont did not answer true for the miniconsole")
  assert(setFont("gridMini", family) == true, "setFont did not answer true for an unchanged family")
  local miniSize, miniSizeMsg = getUserWindowSize("gridMini")
  assert(miniSize == nil and type(miniSizeMsg) == "string", "getUserWindowSize did not refuse the miniconsole with a message")
  local missing, missingMsg = setFontSize("noSuchGridWindow", 10)
  assert(missing == nil and type(missingMsg) == "string", "setFontSize on a missing window did not answer nil and a message")

  assert(openUserWindow("gridWindow") == true, "openUserWindow did not answer true")
  gridWindowGeometry = joined(getWindowGeometry("gridWindow"))
  gridWindowSize = joined(getUserWindowSize("gridWindow"))
  gridWindowCells = joined(getColumnCount("gridWindow"), getRowCount("gridWindow"), getFontSize("gridWindow"))

  createBuffer("gridBuffer")
  gridBufferCells = joined(getColumnCount("gridBuffer"), getRowCount("gridBuffer"), getFontSize("gridBuffer"))
  assert(setFontSize("gridBuffer", 9) == true, "setFontSize did not answer true for the buffer")
  gridFontEvents = table.concat(gridFontEvents, ";")
end)
headlessGrid = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessGrid"), qsl("ok"));

        const auto joined = [](const QList<int>& values) {
            QStringList parts;
            for (const int value : values) {
                parts << QString::number(value);
            }
            return parts.join(QLatin1Char(','));
        };
        // What TTextEdit::getColumnCount() and getRowCount() would count in that font
        const auto cells = [](const QSize& size, const QFont& font) {
            const QFontMetricsF metrics(font);
            return QList<int>{qRound(size.width() / metrics.averageCharWidth()), qRound(size.height() / metrics.lineSpacing())};
        };
        const auto subConsoleFont = [host](const int pointSize) {
            TFontAttributes attributes(host->fontsAntiAlias());
            attributes.mPointSize = pointSize;
            return attributes.makeFont();
        };

        const QFont displayFont = host->getDisplayFont();
        const QFontMetrics displayMetrics(displayFont);
        const QSize mainSize(host->mScreenWidth * displayMetrics.averageCharWidth(), host->mScreenHeight * displayMetrics.height());
        QCOMPARE(luaGlobalString(host, "gridMainSize"), joined({mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridMainGeometry"), joined({0, 0, mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridMainCells"), joined({host->mScreenWidth, host->mScreenHeight}));
        QCOMPARE(luaGlobalString(host, "gridMainFont"), qsl("%1,%2").arg(QFontInfo(displayFont).family()).arg(displayFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "gridMainCharSize"), joined({displayMetrics.horizontalAdvance(QChar('W')), displayMetrics.height()}));

        const QFont miniFont = subConsoleFont(12);
        const QFontMetrics miniMetrics(miniFont);
        QCOMPARE(luaGlobalString(host, "gridMiniCells"), joined(cells(QSize(300, 200), miniFont) << 12));
        QCOMPARE(luaGlobalString(host, "gridMiniCharSize"), joined({miniMetrics.horizontalAdvance(QChar('W')), miniMetrics.height()}));
        QCOMPARE(luaGlobalString(host, "gridMiniBiggerCells"), joined(cells(QSize(300, 200), subConsoleFont(15)) << 15));
        QCOMPARE(luaGlobalString(host, "gridMiniResizedCells"), joined(cells(QSize(600, 400), subConsoleFont(15))));
        const QFontMetrics biggerMiniMetrics(subConsoleFont(15));
        QCOMPARE(luaGlobalString(host, "gridMiniColumnSize"), joined({biggerMiniMetrics.averageCharWidth(), biggerMiniMetrics.height()}));

        QCOMPARE(luaGlobalString(host, "gridWindowGeometry"), joined({0, 0, mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridWindowSize"), joined({mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridWindowCells"), joined(cells(mainSize, subConsoleFont(10)) << 10));

        QCOMPARE(luaGlobalString(host, "gridBufferCells"), joined({0, 0, 14}));
        // As a real view tells scripts of each, and of none for a buffer
        QCOMPARE(luaGlobalString(host, "gridFontEvents"), qsl("gridMini,12;gridMini,15;gridMini,15;gridWindow,10"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
gridFontEvents = {}
function gridSettingChanged(_, setting, family, size)
  gridFontEvents[#gridFontEvents + 1] = table.concat({setting, size}, ",")
end
registerAnonymousEventHandler("sysSettingChanged", "gridSettingChanged")
assert(setFontSize("main", 20) == true, "setFontSize did not answer true for main")
assert(setFontSize("main", 20) == true, "setFontSize did not answer true for an unchanged main size")
gridMainFontEvents = table.concat(gridFontEvents, ";")
gridMainBiggerSize = table.concat({getMainWindowSize()}, ",")
gridMainBiggerCells = table.concat({getColumnCount("main"), getRowCount("main"), getFontSize("main")}, ",")
)lua")));
        const QFontMetrics biggerMetrics(host->getDisplayFont());
        QCOMPARE(luaGlobalString(host, "gridMainBiggerSize"), joined({host->mScreenWidth * biggerMetrics.averageCharWidth(), host->mScreenHeight * biggerMetrics.height()}));
        QCOMPARE(luaGlobalString(host, "gridMainBiggerCells"), joined({host->mScreenWidth, host->mScreenHeight, 20}));
        QCOMPARE(luaGlobalString(host, "gridMainFontEvents"), qsl("main,20;main window font,20;editor font,20"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
gridFontEvents = {}
gridBumped = false
function gridBumpMainFont(_, window)
  if window == "main" and not gridBumped then
    gridBumped = true
    setFontSize("main", 22)
  end
end
registerAnonymousEventHandler("sysFontChangeEvent", "gridBumpMainFont")
assert(setFontSize("main", 21) == true, "setFontSize did not answer true for main")
gridReentrantFontEvents = table.concat(gridFontEvents, ";")
)lua")));
        // As Host::updateConsolesFont() does, the outer change reports the font a handler left, not the one it set
        QCOMPARE(luaGlobalString(host, "gridReentrantFontEvents"), qsl("main,21;main,22;main window font,22;editor font,22;main window font,22;editor font,22"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Asking for the geometry created a widget.");
    }

    // What a profile with no view promises scripts: each window call answers as a real view would,
    // from the models alone, on a profile nothing else has touched.
    void test_headlessContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Contract");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(!host->mainConsoleShowsTimeStamps(), "A new profile shows time stamps.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessContract = "not run"
local ok, err = pcall(function()
  assert(echo("main", "contract line\n") == true, "echo to main did not answer true")
  contractGrid = table.concat({getColumnCount(), getRowCount()}, ",")
  contractTimeStamps = tostring(timeStampsEnabled())
  -- "[count]" and then each value, so no values reads apart from an explicit nil
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local function answer(f, ...)
    return described(pcall(f, ...))
  end
  local function joined(...)
    return table.concat({...}, ",")
  end
  local missing = "noSuchContractWindow"
  local answers = {}
  for _, call in ipairs({
    {"echo", echo, missing, "text"},
    {"moveWindow", moveWindow, missing, 1, 1},
    {"resizeWindow", resizeWindow, missing, 10, 10},
    {"showWindow", showWindow, missing},
    {"hideWindow", hideWindow, missing},
    {"windowVisible", windowVisible, missing},
    {"windowType", windowType, missing},
    {"getWindowGeometry", getWindowGeometry, missing},
    {"getColumnCount", getColumnCount, missing},
    {"getRowCount", getRowCount, missing},
    {"getFont", getFont, missing},
    {"getFontSize", getFontSize, missing},
    {"setFontSize", setFontSize, missing, 10},
    {"calcFontSize", calcFontSize, missing},
    {"clearWindow", clearWindow, missing},
    {"getLineCount", getLineCount, missing},
    {"setBackgroundColor", setBackgroundColor, missing, 1, 2, 3, 255},
    {"getBackgroundColor", getBackgroundColor, missing},
    {"getLabelText", getLabelText, missing},
    {"setUserWindowTitle", setUserWindowTitle, missing, "title"},
    {"timeStampsEnabled", timeStampsEnabled, missing},
  }) do
    answers[#answers + 1] = call[1] .. "=" .. answer(unpack(call, 2))
  end
  contractMissing = table.concat(answers, ";")

  assert(createMiniConsole("contractMini", 10, 20, 200, 100) == true, "createMiniConsole did not answer true")
  local miniEcho = answer(echo, "contractMini", "mini line\n")
  local miniType = answer(windowType, "contractMini")
  contractMini = joined(miniEcho, miniType, answer(windowVisible, "contractMini"), joined(getWindowGeometry("contractMini")), getLineCount("contractMini"))
  assert(deleteMiniConsole("contractMini") == true, "deleteMiniConsole did not answer true")
  contractMiniGone = joined(answer(echo, "contractMini", "text"), answer(windowType, "contractMini"))

  assert(createLabel("contractLabel", 5, 6, 70, 80, 1) == true, "createLabel did not answer true")
  local before = getLabelText("contractLabel")
  local labelEcho = answer(echo, "contractLabel", "label words")
  contractLabel = joined(before, labelEcho, getLabelText("contractLabel"), getLabelStyleSheet("contractLabel"), joined(getBackgroundColor("contractLabel")), joined(getWindowGeometry("contractLabel")), answer(windowVisible, "contractLabel"), answer(windowType, "contractLabel"))
  deleteLabel("contractLabel")
end)
headlessContract = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessContract"), qsl("ok"));
        QVERIFY2(mainBufferHolds(host, qsl("contract line")), "echo() to main never reached the main console model.");
        // The grid NAWS reports, which with no view draws no time stamp gutter
        QCOMPARE(luaGlobalString(host, "contractGrid"), qsl("%1,%2").arg(host->mScreenWidth).arg(host->mScreenHeight));
        QCOMPARE(luaGlobalString(host, "contractTimeStamps"), qsl("false"));
        // Taken from the same Lua run in the GUI self-test profile: what a real view answers for a name it
        // has no window for, where showWindow answers false rather than nil and a message
        const QStringList missingAnswers{qsl("echo=[2]nil:msg"),
                                         qsl("moveWindow=[2]nil:msg"),
                                         qsl("resizeWindow=[2]nil:msg"),
                                         qsl("showWindow=[1]false"),
                                         qsl("hideWindow=[2]nil:msg"),
                                         qsl("windowVisible=[2]nil:msg"),
                                         qsl("windowType=[2]nil:msg"),
                                         qsl("getWindowGeometry=[2]nil:msg"),
                                         qsl("getColumnCount=[2]nil:msg"),
                                         qsl("getRowCount=[2]nil:msg"),
                                         qsl("getFont=[2]nil:msg"),
                                         qsl("getFontSize=[2]nil:msg"),
                                         qsl("setFontSize=[2]nil:msg"),
                                         qsl("calcFontSize=[2]nil:msg"),
                                         qsl("clearWindow=[2]nil:msg"),
                                         qsl("getLineCount=[2]nil:msg"),
                                         qsl("setBackgroundColor=[2]nil:msg"),
                                         qsl("getBackgroundColor=[2]nil:msg"),
                                         qsl("getLabelText=[2]nil:msg"),
                                         qsl("setUserWindowTitle=[2]nil:msg"),
                                         qsl("timeStampsEnabled=[2]nil:msg")};
        QCOMPARE(luaGlobalString(host, "contractMissing"), missingAnswers.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "contractMini"), qsl("[1]true,[1]miniconsole,[1]true,10,20,200,100,1"));
        QCOMPARE(luaGlobalString(host, "contractMiniGone"), qsl("[2]nil:msg,[2]nil:msg"));
        QCOMPARE(luaGlobalString(host, "contractLabel"), qsl(",[1]true,label words,background-color: rgba(32, 32, 32, 255);,32,32,32,255,5,6,70,80,[1]true,[1]label"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Keeping the contract created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile.
    void test_headlessProfileStyleSheetContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Profile-Sheet");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessProfileSheet = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  step("openA", openUserWindow, "psA", false, false, "f")
  step("openB", openUserWindow, "psB", true)
  step("ownB", setUserWindowStyleSheet, "psB", "color: blue;")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("set", setProfileStyleSheet, "QWidget { color: rgb(1,2,3); }")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("openC", openUserWindow, "psC", false, false, "f")
  step("sheetC", getUserWindowStyleSheet, "psC")
  step("ownA", setUserWindowStyleSheet, "psA", "color: green;")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("closeB", closeUserWindow, "psB")
  step("set", setProfileStyleSheet, "QWidget { color: red; }")
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("reopenB", openUserWindow, "psB", true)
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("mini", createMiniConsole, "psMini", 0, 0, 10, 10)
  step("sheetMini", getUserWindowStyleSheet, "psMini")
  step("same", setProfileStyleSheet, "QWidget { color: red; }")
  step("unclosed", setProfileStyleSheet, "QWidget { color: ")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("number", setProfileStyleSheet, 5)
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("none", setProfileStyleSheet)
  step("empty", setProfileStyleSheet, "")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("sheetC", getUserWindowStyleSheet, "psC")
  headlessProfileSheetAnswers = table.concat(answers, "\n")
end)
headlessProfileSheet = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessProfileSheet"), qsl("ok"));
        // Every user window takes the profile sheet, a closed one too, until one sets its own; a mini console has none to read
        const QStringList answers{qsl("openA=[1]true"),
                                  qsl("openB=[1]true"),
                                  qsl("ownB=[1]true"),
                                  qsl("sheetA=[1]"),
                                  qsl("sheetB=[1]color: blue;"),
                                  qsl("set=[1]true"),
                                  qsl("sheetA=[1]QWidget { color: rgb(1,2,3); }"),
                                  qsl("sheetB=[1]QWidget { color: rgb(1,2,3); }"),
                                  qsl("openC=[1]true"),
                                  qsl("sheetC=[1]QWidget { color: rgb(1,2,3); }"),
                                  qsl("ownA=[1]true"),
                                  qsl("sheetA=[1]color: green;"),
                                  qsl("closeB=[1]true"),
                                  qsl("set=[1]true"),
                                  qsl("sheetB=[1]QWidget { color: red; }"),
                                  qsl("reopenB=[1]true"),
                                  qsl("sheetB=[1]QWidget { color: red; }"),
                                  qsl("sheetA=[1]QWidget { color: red; }"),
                                  qsl("mini=[1]true"),
                                  qsl("sheetMini=[2]nil:msg"),
                                  qsl("same=[1]true"),
                                  qsl("unclosed=[1]true"),
                                  qsl("sheetA=[1]QWidget { color: "),
                                  qsl("number=[1]true"),
                                  qsl("sheetA=[1]5"),
                                  qsl("none=raised"),
                                  qsl("empty=[1]true"),
                                  qsl("sheetA=[1]"),
                                  qsl("sheetC=[1]")};
        QCOMPARE(luaGlobalString(host, "headlessProfileSheetAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Setting the profile sheet created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, with files of the same bytes.
    void test_headlessLabelMovieContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Label-Movie");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir movieDir;
        QVERIFY(movieDir.isValid());
        const auto writeFile = [&movieDir](const QString& name, const QByteArray& data) {
            QFile file(movieDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        // Three 40x16 frames, then a 24x10 still image
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("fake.gif"), QByteArrayLiteral("not an image")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("mvDir = [==[%1]==]").arg(movieDir.path())));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessMovie = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local startGifs = getProfileStats().gifs
  local function state(label, name)
    local gifs = getProfileStats().gifs
    local width, height = getLabelSizeHint(name)
    answers[#answers + 1] = string.format("%s:hint=%sx%s,text=%s,gifs=%d/%d", label, tostring(width), tostring(height), tostring(getLabelText(name)), gifs.total - startGifs.total, gifs.active - startGifs.active)
  end
  local function textState(label, name, twin)
    local gifs = getProfileStats().gifs
    local width, height = getLabelSizeHint(name)
    local twinWidth, twinHeight = getLabelSizeHint(twin)
    local hint = (width == twinWidth and height == twinHeight) and twin or string.format("%sx%s", tostring(width), tostring(height))
    answers[#answers + 1] = string.format("%s:hint=%s,text=%s,gifs=%d/%d", label, hint, tostring(getLabelText(name)), gifs.total - startGifs.total, gifs.active - startGifs.active)
  end
  local gif, still, fake = mvDir .. "/anim.gif", mvDir .. "/still.png", mvDir .. "/fake.gif"
  assert(createLabel("mvA", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(createLabel("mvB", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  step("echo", echo, "mvA", "words")
  for _, name in ipairs({"startMovie", "pauseMovie", "scaleMovie"}) do
    step(name, _G[name], "mvA")
  end
  step("setMovieFrame", setMovieFrame, "mvA", 1)
  step("setMovieSpeed", setMovieSpeed, "mvA", 50)
  step("startMissing", startMovie, "mvNoSuch")
  step("startEmpty", startMovie, "")
  step("startNumber", startMovie, 5)
  step("setMissing", setMovie, "mvNoSuch", gif)
  step("setEmpty", setMovie, "", gif)
  step("setNoFile", setMovie, "mvA", mvDir .. "/nosuch.gif")
  step("setNotImage", setMovie, "mvA", fake)
  step("setStill", setMovie, "mvA", still)
  state("still", "mvA")
  step("startStill", startMovie, "mvA")
  step("set", setMovie, "mvA", gif)
  state("set", "mvA")
  step("start", startMovie, "mvA")
  step("pause", pauseMovie, "mvA")
  state("paused", "mvA")
  step("frame0", setMovieFrame, "mvA", 0)
  step("frame2", setMovieFrame, "mvA", 2)
  step("frameNegative", setMovieFrame, "mvA", -1)
  step("framePast", setMovieFrame, "mvA", 3)
  step("frameText", setMovieFrame, "mvA", "x")
  step("speed", setMovieSpeed, "mvA", 200)
  step("speedText", setMovieSpeed, "mvA", "x")
  step("scale", scaleMovie, "mvA")
  state("scaled", "mvA")
  step("setScaled", setMovie, "mvA", gif)
  state("setScaled", "mvA")
  step("pauseScaled", pauseMovie, "mvA")
  step("frame1", setMovieFrame, "mvA", 1)
  state("scaledFrame", "mvA")
  step("resize", resizeWindow, "mvA", 200, 40)
  step("frame2", setMovieFrame, "mvA", 2)
  state("resizedFrame", "mvA")
  step("setFollowed", setMovie, "mvA", gif)
  state("followed", "mvA")
  step("unscale", scaleMovie, "mvA", false)
  step("scaleText", scaleMovie, "mvA", "x")
  step("resize", resizeWindow, "mvA", 50, 10)
  step("frame0", setMovieFrame, "mvA", 0)
  state("unscaledFrame", "mvA")
  step("start", startMovie, "mvA")
  step("setB", setMovie, "mvB", gif)
  state("two", "mvB")
  step("setAgain", setMovie, "mvA", gif)
  state("again", "mvA")
  step("setNotImageAfter", setMovie, "mvA", fake)
  step("startAfter", startMovie, "mvA")
  step("echoEmpty", echo, "mvA", "")
  state("echoEmpty", "mvA")
  step("startEchoEmpty", startMovie, "mvA")
  state("startEchoEmpty", "mvA")
  step("echoText", echo, "mvA", "again")
  assert(createLabel("mvText", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  echo("mvText", "again")
  textState("echoText", "mvA", "mvText")
  deleteLabel("mvText")
  step("startEchoText", startMovie, "mvA")
  step("setText", setMovie, "mvA", gif)
  step("sheet", setLabelStyleSheet, "mvA", "padding: 3px; border: 2px solid red;")
  state("sheet", "mvA")
  assert(openUserWindow("mvWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("move", setWindow, "mvWindow", "mvA", 0, 0, true)
  step("startMoved", startMovie, "mvA")
  step("hide", hideWindow, "mvA")
  step("pauseHidden", pauseMovie, "mvA")
  state("hidden", "mvA")
  step("scaleHidden", scaleMovie, "mvA")
  step("resizeHidden", resizeWindow, "mvA", 120, 30)
  step("setHidden", setMovie, "mvA", gif)
  state("resizedHidden", "mvA")
  step("show", showWindow, "mvA")
  step("setShown", setMovie, "mvA", gif)
  state("resizedShown", "mvA")
  step("delete", deleteLabel, "mvA")
  step("startDeleted", startMovie, "mvA")
  state("deleted", "mvB")
  step("deleteB", deleteLabel, "mvB")
  local gifs = getProfileStats().gifs
  answers[#answers + 1] = string.format("gone:gifs=%d/%d", gifs.total - startGifs.total, gifs.active - startGifs.active)
  closeUserWindow("mvWindow")
  headlessMovieAnswers = table.concat(answers, "\n"):gsub(mvDir:gsub("%p", "%%%0"), "<dir>")
end)
headlessMovie = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMovie"), qsl("ok"));
        // A text label drops the movie but keeps it for the next setMovie(); a movie's size hint is its frame's
        const QStringList answers{qsl("echo=[1]true"),
                                  qsl("startMovie=[2]nil:no movie found at label 'mvA'"),
                                  qsl("pauseMovie=[2]nil:no movie found at label 'mvA'"),
                                  qsl("scaleMovie=[2]nil:no movie found at label 'mvA'"),
                                  qsl("setMovieFrame=[2]nil:no movie found at label 'mvA'"),
                                  qsl("setMovieSpeed=[2]nil:no movie found at label 'mvA'"),
                                  qsl("startMissing=[2]nil:label \"mvNoSuch\" not found"),
                                  qsl("startEmpty=[2]nil:label name cannot be an empty string"),
                                  qsl("startNumber=[2]nil:label \"5\" not found"),
                                  qsl("setMissing=[2]nil:label 'mvNoSuch' does not exist"),
                                  qsl("setEmpty=[2]nil:label name cannot be an empty string"),
                                  qsl("setNoFile=[2]nil:no valid movie found at '<dir>/nosuch.gif'"),
                                  qsl("setNotImage=[2]nil:no valid movie found at '<dir>/fake.gif'"),
                                  qsl("setStill=[1]true"),
                                  qsl("still:hint=24x10,text=,gifs=1/1"),
                                  qsl("startStill=[1]true"),
                                  qsl("set=[1]true"),
                                  qsl("set:hint=40x16,text=,gifs=1/1"),
                                  qsl("start=[1]true"),
                                  qsl("pause=[1]true"),
                                  qsl("paused:hint=40x16,text=,gifs=1/0"),
                                  qsl("frame0=[1]true"),
                                  qsl("frame2=[1]true"),
                                  qsl("frameNegative=[1]false"),
                                  qsl("framePast=[1]true"),
                                  qsl("frameText=raised"),
                                  qsl("speed=[1]true"),
                                  qsl("speedText=raised"),
                                  qsl("scale=[1]true"),
                                  qsl("scaled:hint=40x16,text=,gifs=1/0"),
                                  qsl("setScaled=[1]true"),
                                  qsl("setScaled:hint=100x20,text=,gifs=1/1"),
                                  qsl("pauseScaled=[1]true"),
                                  qsl("frame1=[1]true"),
                                  qsl("scaledFrame:hint=100x20,text=,gifs=1/0"),
                                  qsl("resize=[1]true"),
                                  qsl("frame2=[1]true"),
                                  qsl("resizedFrame:hint=200x40,text=,gifs=1/0"),
                                  qsl("setFollowed=[1]true"),
                                  qsl("followed:hint=200x40,text=,gifs=1/1"),
                                  qsl("unscale=[1]true"),
                                  qsl("scaleText=raised"),
                                  qsl("resize=[1]true"),
                                  qsl("frame0=[1]true"),
                                  qsl("unscaledFrame:hint=200x40,text=,gifs=1/1"),
                                  qsl("start=[1]true"),
                                  qsl("setB=[1]true"),
                                  qsl("two:hint=40x16,text=,gifs=2/2"),
                                  qsl("setAgain=[1]true"),
                                  qsl("again:hint=200x40,text=,gifs=2/2"),
                                  qsl("setNotImageAfter=[2]nil:no valid movie found at '<dir>/fake.gif'"),
                                  qsl("startAfter=[1]true"),
                                  qsl("echoEmpty=[1]true"),
                                  qsl("echoEmpty:hint=200x40,text=,gifs=2/1"),
                                  qsl("startEchoEmpty=[1]true"),
                                  qsl("startEchoEmpty:hint=200x40,text=,gifs=2/2"),
                                  qsl("echoText=[1]true"),
                                  qsl("echoText:hint=mvText,text=again,gifs=2/1"),
                                  qsl("startEchoText=[2]nil:no movie found at label 'mvA'"),
                                  qsl("setText=[1]true"),
                                  qsl("sheet=[1]true"),
                                  qsl("sheet:hint=210x50,text=,gifs=2/2"),
                                  qsl("move=[1]true"),
                                  qsl("startMoved=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("pauseHidden=[1]true"),
                                  qsl("hidden:hint=210x50,text=,gifs=2/1"),
                                  qsl("scaleHidden=[1]true"),
                                  qsl("resizeHidden=[1]true"),
                                  qsl("setHidden=[1]true"),
                                  qsl("resizedHidden:hint=60x20,text=,gifs=2/2"),
                                  qsl("show=[1]true"),
                                  qsl("setShown=[1]true"),
                                  qsl("resizedShown:hint=130x40,text=,gifs=2/2"),
                                  qsl("delete=[1]true"),
                                  qsl("startDeleted=[2]nil:label \"mvA\" not found"),
                                  qsl("deleted:hint=40x16,text=,gifs=1/1"),
                                  qsl("deleteB=[1]true"),
                                  qsl("gone:gifs=0/0")};
        QCOMPARE(luaGlobalString(host, "headlessMovieAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Playing the movies created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, with files of the same bytes.
    void test_headlessBackgroundImageContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Background-Image");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir imageDir;
        QVERIFY(imageDir.isValid());
        const auto writeFile = [&imageDir](const QString& name, const QByteArray& data) {
            QFile file(imageDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        // Three 40x16 frames, then a 24x10 still image
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("fake.gif"), QByteArrayLiteral("not an image")));
        QVERIFY(writeFile(qsl("sized.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"30\" height=\"12\"><rect width=\"30\" height=\"12\" fill=\"red\"/></svg>\n")));
        QVERIFY(writeFile(qsl("nosize.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"><rect width=\"5\" height=\"5\" fill=\"red\"/></svg>\n")));
        QVERIFY(writeFile(qsl("bad.svg"), QByteArrayLiteral("<notsvg>\n")));
        QVERIFY(writeFile(qsl("blank.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"/>\n")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("bgDir = [==[%1]==]").arg(imageDir.path())));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessBackground = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local startGifs = getProfileStats().gifs
  local sheet = ""
  local function state(label, name, emptied)
    local gifs = getProfileStats().gifs
    local width, height = getLabelSizeHint(name)
    local text = getLabelText(name)
    -- What a label measures for text, or for nothing, depends on the font, so that is read against a label with only the same text
    createLabel("bgTwin", 0, 0, 100, 20, 1)
    if sheet ~= "" then
      setLabelStyleSheet("bgTwin", sheet)
    end
    -- A label whose text was emptied measures a line, which a fresh one does not
    if emptied then
      echo("bgTwin", "words")
      echo("bgTwin", "")
    end
    if text ~= "" then
      echo("bgTwin", text)
    end
    local twinWidth, twinHeight = getLabelSizeHint("bgTwin")
    deleteLabel("bgTwin")
    local hint = (width == twinWidth and height == twinHeight) and "twin" or string.format("%sx%s", tostring(width), tostring(height))
    local movie = setMovieSpeed(name, 100) == true
    answers[#answers + 1] = string.format("%s:hint=%s,text=%s,movie=%s,gifs=%d/%d", label, hint, tostring(text), tostring(movie), gifs.total - startGifs.total, gifs.active - startGifs.active)
  end
  local gif, still, fake = bgDir .. "/anim.gif", bgDir .. "/still.png", bgDir .. "/fake.gif"
  local sized, nosize, bad = bgDir .. "/sized.svg", bgDir .. "/nosize.svg", bgDir .. "/bad.svg"
  local blank = bgDir .. "/blank.svg"
  assert(createLabel("bgA", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  state("fresh", "bgA")
  step("still", setBackgroundImage, "bgA", still)
  state("still", "bgA")
  step("missing", setBackgroundImage, "bgA", bgDir .. "/nosuch.png")
  step("notImage", setBackgroundImage, "bgA", fake)
  step("badSvg", setBackgroundImage, "bgA", bad)
  state("kept", "bgA")
  step("empty", setBackgroundImage, "bgA", "")
  state("empty", "bgA")
  step("emptyAgain", setBackgroundImage, "bgA", "")
  step("echoText", echo, "bgA", "words")
  step("emptyOnText", setBackgroundImage, "bgA", "")
  state("emptyOnText", "bgA")
  step("mode3", setBackgroundImage, "bgA", still, 3)
  step("mode4", setBackgroundImage, "bgA", "background-color: red;", 4)
  step("mode5", setBackgroundImage, "bgA", still, 5)
  step("mode0", setBackgroundImage, "bgA", still, 0)
  step("full", setBackgroundImage, "bgA", still, 1, true)
  step("echoEmpty", echo, "bgA", "")
  state("echoEmpty", "bgA")
  step("echoText", echo, "bgA", "words")
  state("echoText", "bgA")
  step("onText", setBackgroundImage, "bgA", still)
  state("onText", "bgA")
  step("resetImage", resetBackgroundImage, "bgA")
  state("resetImage", "bgA")
  step("resetAgain", resetBackgroundImage, "bgA")
  step("echoText", echo, "bgA", "words")
  step("resetText", resetBackgroundImage, "bgA")
  state("resetText", "bgA")
  step("setMovie", setMovie, "bgA", gif)
  state("movie", "bgA")
  step("onMovie", setBackgroundImage, "bgA", still)
  state("onMovie", "bgA")
  step("startMovie", startMovie, "bgA")
  step("setMovieAgain", setMovie, "bgA", gif)
  state("movieAgain", "bgA")
  step("emptyOnMovie", setBackgroundImage, "bgA", "")
  state("emptyOnMovie", "bgA")
  step("setMovieAgain", setMovie, "bgA", gif)
  step("resetMovie", resetBackgroundImage, "bgA")
  state("resetMovie", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  state("sized", "bgA")
  step("echoOnSvg", echo, "bgA", "svg text")
  state("echoOnSvg", "bgA")
  step("echoEmptyOnSvg", echo, "bgA", "")
  state("echoEmptyOnSvg", "bgA")
  step("rasterOnSvg", setBackgroundImage, "bgA", still)
  state("rasterOnSvg", "bgA")
  step("svgOnRaster", setBackgroundImage, "bgA", sized)
  state("svgOnRaster", "bgA")
  step("badOnSvg", setBackgroundImage, "bgA", bad)
  state("badOnSvg", "bgA")
  step("nosize", setBackgroundImage, "bgA", nosize)
  state("nosize", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("movieOnSvg", setMovie, "bgA", gif)
  state("movieOnSvg", "bgA")
  step("svgOnMovie", setBackgroundImage, "bgA", nosize)
  state("svgOnMovie", "bgA")
  step("rasterOnSvgMovie", setBackgroundImage, "bgA", still)
  state("rasterOnSvgMovie", "bgA")
  step("movieOnRaster", setMovie, "bgA", gif)
  state("movieOnRaster", "bgA")
  step("svgOnRasterMovie", setBackgroundImage, "bgA", sized)
  state("svgOnRasterMovie", "bgA")
  step("resetSvgMovie", resetBackgroundImage, "bgA")
  state("resetSvgMovie", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("resetSvg", resetBackgroundImage, "bgA")
  state("resetSvg", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("emptyOnSvg", setBackgroundImage, "bgA", "")
  state("emptyOnSvg", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("still", setBackgroundImage, "bgA", still)
  step("echoText", echo, "bgA", "words")
  step("echoEmpty", echo, "bgA", "")
  state("rasterDroppedSvg", "bgA", true)
  step("blank", setBackgroundImage, "bgA", blank)
  state("blank", "bgA", true)
  step("sized", setBackgroundImage, "bgA", sized)
  sheet = "padding: 3px; border: 2px solid red;"
  step("sheet", setLabelStyleSheet, "bgA", sheet)
  state("sheetSvg", "bgA")
  step("still", setBackgroundImage, "bgA", still)
  state("sheetStill", "bgA")
  step("echoText", echo, "bgA", "words")
  step("svgOnText", setBackgroundImage, "bgA", sized)
  state("svgOnText", "bgA")
  step("resetSvgText", resetBackgroundImage, "bgA")
  state("resetSvgText", "bgA")
  step("missingLabel", setBackgroundImage, "bgNoSuch", still)
  step("resetMissing", resetBackgroundImage, "bgNoSuch")
  step("emptyName", setBackgroundImage, "", still)
  step("number", setBackgroundImage, 5, still)
  step("noArgs", setBackgroundImage)
  step("main", setBackgroundImage, "main", still)
  step("mainMissing", setBackgroundImage, "main", bgDir .. "/nosuch.png")
  step("mainMode2", setBackgroundImage, "main", still, 2)
  step("mainMode4", setBackgroundImage, "main", "background-color: red;", 4)
  step("pathOnly", setBackgroundImage, still)
  step("pathMode", setBackgroundImage, still, 3)
  step("mainReset", resetBackgroundImage, "main")
  step("resetNoArgs", resetBackgroundImage)
  step("fullStill", setBackgroundImage, "main", still, 1, true)
  step("fullCover", setBackgroundImage, "main", still, 5, true)
  step("fullCoverMissing", setBackgroundImage, "main", bgDir .. "/nosuch.png", 5, true)
  step("fullCoverFake", setBackgroundImage, "main", fake, 5, true)
  step("fullMissing", setBackgroundImage, "main", bgDir .. "/nosuch.png", 1, true)
  step("fullReset", resetBackgroundImage, "main", true)
  step("fullResetName", resetBackgroundImage, "bgA", true)
  step("fullOther", setBackgroundImage, "bgA", still, 1, true)
  assert(createMiniConsole("bgMini", 0, 0, 100, 100) == true, "createMiniConsole did not answer true")
  step("mini", setBackgroundImage, "bgMini", still)
  step("miniMissing", setBackgroundImage, "bgMini", bgDir .. "/nosuch.png")
  step("miniMode5", setBackgroundImage, "bgMini", still, 5)
  step("miniReset", resetBackgroundImage, "bgMini")
  assert(openUserWindow("bgWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("window", setBackgroundImage, "bgWindow", still, 3)
  step("windowReset", resetBackgroundImage, "bgWindow")
  step("closeWindow", closeUserWindow, "bgWindow")
  step("closedWindow", setBackgroundImage, "bgWindow", still)
  step("closedWindowReset", resetBackgroundImage, "bgWindow")
  createBuffer("bgBuffer")
  step("buffer", setBackgroundImage, "bgBuffer", still)
  step("bufferReset", resetBackgroundImage, "bgBuffer")
  step("move", setWindow, "bgWindow", "bgA", 0, 0, true)
  step("movedLabel", setBackgroundImage, "bgA", still)
  state("moved", "bgA")
  step("delete", deleteLabel, "bgA")
  step("deleted", setBackgroundImage, "bgA", still)
  step("deletedReset", resetBackgroundImage, "bgA")
  step("deleteMini", deleteMiniConsole, "bgMini")
  step("deletedMini", setBackgroundImage, "bgMini", still)
  local gifs = getProfileStats().gifs
  answers[#answers + 1] = string.format("gone:gifs=%d/%d", gifs.total - startGifs.total, gifs.active - startGifs.active)
  resetBackgroundImage("main")
  resetBackgroundImage("main", true)
  headlessBackgroundAnswers = table.concat(answers, "\n"):gsub(bgDir:gsub("%p", "%%%0"), "<dir>")
end)
headlessBackground = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessBackground"), qsl("ok"));
        // A raster takes the place of a label's text or movie, an SVG paints under them; a console takes any path
        const QStringList answers{qsl("fresh:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("still=[1]true"),
                                  qsl("still:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("missing=[2]nil:console or label 'bgA' not found, or '<dir>/nosuch.png' could not be loaded as an image"),
                                  qsl("notImage=[2]nil:console or label 'bgA' not found, or '<dir>/fake.gif' could not be loaded as an image"),
                                  qsl("badSvg=[2]nil:console or label 'bgA' not found, or '<dir>/bad.svg' could not be loaded as an image"),
                                  qsl("kept:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("empty=[1]true"),
                                  qsl("empty:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("emptyAgain=[1]true"),
                                  qsl("echoText=[1]true"),
                                  qsl("emptyOnText=[1]true"),
                                  qsl("emptyOnText:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("mode3=[1]true"),
                                  qsl("mode4=[2]nil:console or label 'bgA' not found, or 'background-color: red;' could not be loaded as an image"),
                                  qsl("mode5=[2]nil:mode 'cover' is not supported for the main display - pass true as the 4th argument to apply it to the full window background instead"),
                                  qsl("mode0=[2]nil:0 is not a valid mode! Valid modes are 1 'border', 2 'center', 3 'tile', 4 'style', 5 'cover'"),
                                  qsl("full=[2]nil:the full window background can only be used with the main console"),
                                  qsl("echoEmpty=[1]true"),
                                  qsl("echoEmpty:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("echoText=[1]true"),
                                  qsl("echoText:hint=twin,text=words,movie=false,gifs=0/0"),
                                  qsl("onText=[1]true"),
                                  qsl("onText:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("resetImage=[1]true"),
                                  qsl("resetImage:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("resetAgain=[1]true"),
                                  qsl("echoText=[1]true"),
                                  qsl("resetText=[1]true"),
                                  qsl("resetText:hint=twin,text=words,movie=false,gifs=0/0"),
                                  qsl("setMovie=[1]true"),
                                  qsl("movie:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("onMovie=[1]true"),
                                  qsl("onMovie:hint=24x10,text=,movie=false,gifs=1/0"),
                                  qsl("startMovie=[2]nil:no movie found at label 'bgA'"),
                                  qsl("setMovieAgain=[1]true"),
                                  qsl("movieAgain:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("emptyOnMovie=[1]true"),
                                  qsl("emptyOnMovie:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("setMovieAgain=[1]true"),
                                  qsl("resetMovie=[1]true"),
                                  qsl("resetMovie:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("sized:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("echoOnSvg=[1]true"),
                                  qsl("echoOnSvg:hint=twin,text=svg text,movie=false,gifs=1/0"),
                                  qsl("echoEmptyOnSvg=[1]true"),
                                  qsl("echoEmptyOnSvg:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("rasterOnSvg=[1]true"),
                                  qsl("rasterOnSvg:hint=24x10,text=,movie=false,gifs=1/0"),
                                  qsl("svgOnRaster=[1]true"),
                                  qsl("svgOnRaster:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("badOnSvg=[2]nil:console or label 'bgA' not found, or '<dir>/bad.svg' could not be loaded as an image"),
                                  qsl("badOnSvg:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("nosize=[1]true"),
                                  qsl("nosize:hint=5x5,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("movieOnSvg=[1]true"),
                                  qsl("movieOnSvg:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("svgOnMovie=[1]true"),
                                  qsl("svgOnMovie:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("rasterOnSvgMovie=[1]true"),
                                  qsl("rasterOnSvgMovie:hint=24x10,text=,movie=false,gifs=1/0"),
                                  qsl("movieOnRaster=[1]true"),
                                  qsl("movieOnRaster:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("svgOnRasterMovie=[1]true"),
                                  qsl("svgOnRasterMovie:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("resetSvgMovie=[1]true"),
                                  qsl("resetSvgMovie:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("resetSvg=[1]true"),
                                  qsl("resetSvg:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("emptyOnSvg=[1]true"),
                                  qsl("emptyOnSvg:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("still=[1]true"),
                                  qsl("echoText=[1]true"),
                                  qsl("echoEmpty=[1]true"),
                                  qsl("rasterDroppedSvg:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("blank=[1]true"),
                                  qsl("blank:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("sheet=[1]true"),
                                  qsl("sheetSvg:hint=40x22,text=,movie=false,gifs=1/0"),
                                  qsl("still=[1]true"),
                                  qsl("sheetStill:hint=34x20,text=,movie=false,gifs=1/0"),
                                  qsl("echoText=[1]true"),
                                  qsl("svgOnText=[1]true"),
                                  qsl("svgOnText:hint=twin,text=words,movie=false,gifs=1/0"),
                                  qsl("resetSvgText=[1]true"),
                                  qsl("resetSvgText:hint=twin,text=words,movie=false,gifs=1/0"),
                                  qsl("missingLabel=[2]nil:console or label 'bgNoSuch' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("resetMissing=[2]nil:console 'bgNoSuch' not found"),
                                  qsl("emptyName=[1]true"),
                                  qsl("number=[2]nil:console or label '5' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("noArgs=raised"),
                                  qsl("main=[1]true"),
                                  qsl("mainMissing=[1]true"),
                                  qsl("mainMode2=[1]true"),
                                  qsl("mainMode4=[1]true"),
                                  qsl("pathOnly=[1]true"),
                                  qsl("pathMode=[1]true"),
                                  qsl("mainReset=[1]true"),
                                  qsl("resetNoArgs=[1]true"),
                                  qsl("fullStill=[1]true"),
                                  qsl("fullCover=[1]true"),
                                  qsl("fullCoverMissing=[2]nil:could not use '<dir>/nosuch.png' as a full window background image"),
                                  qsl("fullCoverFake=[2]nil:could not use '<dir>/fake.gif' as a full window background image"),
                                  qsl("fullMissing=[1]true"),
                                  qsl("fullReset=[1]true"),
                                  qsl("fullResetName=[2]nil:the full window background can only be reset on the main console"),
                                  qsl("fullOther=[2]nil:the full window background can only be used with the main console"),
                                  qsl("mini=[1]true"),
                                  qsl("miniMissing=[1]true"),
                                  qsl("miniMode5=[2]nil:mode 'cover' is not supported for the main display - pass true as the 4th argument to apply it to the full window background instead"),
                                  qsl("miniReset=[1]true"),
                                  qsl("window=[1]true"),
                                  qsl("windowReset=[1]true"),
                                  qsl("closeWindow=[1]true"),
                                  qsl("closedWindow=[1]true"),
                                  qsl("closedWindowReset=[1]true"),
                                  qsl("buffer=[1]true"),
                                  qsl("bufferReset=[1]true"),
                                  qsl("move=[1]true"),
                                  qsl("movedLabel=[1]true"),
                                  qsl("moved:hint=34x20,text=,movie=false,gifs=1/0"),
                                  qsl("delete=[1]true"),
                                  qsl("deleted=[2]nil:console or label 'bgA' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("deletedReset=[2]nil:console 'bgA' not found"),
                                  qsl("deleteMini=[1]true"),
                                  qsl("deletedMini=[2]nil:console or label 'bgMini' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("gone:gifs=0/0")};
        QCOMPARE(luaGlobalString(host, "headlessBackgroundAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Setting the background images created a widget.");
    }

    // The answers are those GeyserLabel_spec.lua gets in the GUI self-test profile
    void test_headlessResetTakesStyleSheetImage()
    {
        const QString hostname = qsl("Test-Headless-Host-Sheet-Image");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local answers = {}
createLabel("sheetImage", 0, 0, 40, 20, 1)
for _, sheet in ipairs({
  "border: 1px solid red; background-image: url(data:image/png;base64,AAAA); color: blue;",
  "QLabel{color: red;} QLabel::hover{background-image: url(hover.png);}",
  "Background-Image: url(:/icons/mudlet.png); color: blue;",
}) do
  setLabelStyleSheet("sheetImage", sheet)
  answers[#answers + 1] = tostring(resetBackgroundImage("sheetImage")) .. ":" .. getLabelStyleSheet("sheetImage")
end
headlessSheetImageAnswers = table.concat(answers, "\n")
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        const QStringList answers{qsl("true:border: 1px solid red;  color: blue;"), qsl("true:QLabel{color: red;} QLabel::hover{background-image: url(hover.png);}"), qsl("true: color: blue;")};
        QCOMPARE(luaGlobalString(host, "headlessSheetImageAnswers"), answers.join(QLatin1Char('\n')));
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessRaiseLowerContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Raise-Lower");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessRaise = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function both(label, name)
    step(label .. "Raise", raiseWindow, name)
    step(label .. "Lower", lowerWindow, name)
  end
  assert(createLabel("rlLabel", 0, 0, 50, 20, 1) == true, "createLabel did not answer true")
  assert(createMiniConsole("rlMini", 0, 0, 100, 100) == true, "createMiniConsole did not answer true")
  assert(openUserWindow("rlWindow", false, false, "f") == true, "openUserWindow did not answer true")
  createBuffer("rlBuffer")
  assert(createLabel("rlWindow", "rlInner", 0, 0, 50, 20, 1) == true, "createLabel in a user window did not answer true")
  assert(createMiniConsole("rlWindow", "rlInnerMini", 0, 0, 50, 50) == true, "createMiniConsole in a user window did not answer true")
  both("label", "rlLabel")
  both("mini", "rlMini")
  both("window", "rlWindow")
  both("buffer", "rlBuffer")
  both("innerLabel", "rlInner")
  both("innerMini", "rlInnerMini")
  both("main", "main")
  both("empty", "")
  both("missing", "rlNoSuch")
  both("case", "RLLABEL")
  both("mapper", "mapper")
  both("number", 5)
  step("nilRaise", raiseWindow, nil)
  step("noArgsRaise", raiseWindow)
  step("noArgsLower", lowerWindow)
  step("tableRaise", raiseWindow, {})
  step("tableLower", lowerWindow, {})
  step("hide", hideWindow, "rlLabel")
  both("hiddenLabel", "rlLabel")
  step("hideMini", hideWindow, "rlMini")
  both("hiddenMini", "rlMini")
  step("closeWindow", closeUserWindow, "rlWindow")
  both("closedWindow", "rlWindow")
  both("closedInner", "rlInner")
  step("deleteLabel", deleteLabel, "rlLabel")
  both("deletedLabel", "rlLabel")
  step("deleteMini", deleteMiniConsole, "rlMini")
  both("deletedMini", "rlMini")
  step("deleteInner", deleteLabel, "rlInner")
  step("deleteInnerMini", deleteMiniConsole, "rlInnerMini")
  headlessRaiseAnswers = table.concat(answers, "\n")
end)
headlessRaise = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessRaise"), qsl("ok"));
        const QStringList answers{qsl("labelRaise=[1]true"),
                                  qsl("labelLower=[1]true"),
                                  qsl("miniRaise=[1]true"),
                                  qsl("miniLower=[1]true"),
                                  qsl("windowRaise=[1]true"),
                                  qsl("windowLower=[1]true"),
                                  qsl("bufferRaise=[1]true"),
                                  qsl("bufferLower=[1]true"),
                                  qsl("innerLabelRaise=[1]true"),
                                  qsl("innerLabelLower=[1]true"),
                                  qsl("innerMiniRaise=[1]true"),
                                  qsl("innerMiniLower=[1]true"),
                                  qsl("mainRaise=[1]false"),
                                  qsl("mainLower=[1]false"),
                                  qsl("emptyRaise=[1]false"),
                                  qsl("emptyLower=[1]false"),
                                  qsl("missingRaise=[1]false"),
                                  qsl("missingLower=[1]false"),
                                  qsl("caseRaise=[1]false"),
                                  qsl("caseLower=[1]false"),
                                  qsl("mapperRaise=[1]false"),
                                  qsl("mapperLower=[1]false"),
                                  qsl("numberRaise=[1]false"),
                                  qsl("numberLower=[1]false"),
                                  qsl("nilRaise=[1]false"),
                                  qsl("noArgsRaise=[1]false"),
                                  qsl("noArgsLower=[1]false"),
                                  qsl("tableRaise=raised:raiseWindow: bad argument #1 type (window name as string expected, got table)!"),
                                  qsl("tableLower=raised:lowerWindow: bad argument #1 type (window name as string expected, got table)!"),
                                  qsl("hide=[1]true"),
                                  qsl("hiddenLabelRaise=[1]true"),
                                  qsl("hiddenLabelLower=[1]true"),
                                  qsl("hideMini=[1]true"),
                                  qsl("hiddenMiniRaise=[1]true"),
                                  qsl("hiddenMiniLower=[1]true"),
                                  qsl("closeWindow=[1]true"),
                                  qsl("closedWindowRaise=[1]true"),
                                  qsl("closedWindowLower=[1]true"),
                                  qsl("closedInnerRaise=[1]true"),
                                  qsl("closedInnerLower=[1]true"),
                                  qsl("deleteLabel=[1]true"),
                                  qsl("deletedLabelRaise=[1]false"),
                                  qsl("deletedLabelLower=[1]false"),
                                  qsl("deleteMini=[1]true"),
                                  qsl("deletedMiniRaise=[1]false"),
                                  qsl("deletedMiniLower=[1]false"),
                                  qsl("deleteInner=[1]true"),
                                  qsl("deleteInnerMini=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessRaiseAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Raising and lowering the windows created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones:
    // a spec cannot wait for deferred deletes, which take what a real deleted scroll box held out of
    // the registry as TLabel's and TConsole's destructors and TMainConsole::deregisterScrollBox() run
    void test_headlessScrollBoxContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Scroll-Box");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessScroll = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- A label measures its text in the font it inherits, which is read against labels in the main window and in a user window
  local function font(label, name)
    local width, height = getLabelSizeHint(name)
    local kinds = {}
    for _, parent in ipairs({"main", "sbWindow"}) do
      createLabel(parent, "sbTwin", 0, 0, 50, 20, 1)
      echo("sbTwin", getLabelText(name))
      local twinWidth, twinHeight = getLabelSizeHint("sbTwin")
      deleteLabel("sbTwin")
      if width == twinWidth and height == twinHeight then
        kinds[#kinds + 1] = parent
      end
    end
    answers[#answers + 1] = label .. ":font=" .. (#kinds > 0 and table.concat(kinds, "+") or "other")
  end
  local deletedEvents = {}
  -- By name, as the C++ registerAnonymousEventHandler() takes no function
  function headlessScrollDeleted(_, name)
    deletedEvents[#deletedEvents + 1] = name
  end
  registerAnonymousEventHandler("sysScrollBoxDeleted", "headlessScrollDeleted")
  local function events(label)
    answers[#answers + 1] = label .. ":events=" .. table.concat(deletedEvents, ",")
    deletedEvents = {}
  end
  assert(openUserWindow("sbWindow", false, false, "f") == true, "openUserWindow did not answer true")
  setFont("sbWindow", "Bitstream Vera Sans Mono")
  setFontSize("sbWindow", 20)
  step("create", createScrollBox, "sbBox", 10, 20, 200, 100)
  look("box", "sbBox")
  step("createAgain", createScrollBox, "sbBox", 15, 25, 210, 110)
  look("boxAgain", "sbBox")
  step("createMain", createScrollBox, "main", "sbMainBox", 1, 2, 30, 40)
  look("mainBox", "sbMainBox")
  step("createInWindow", createScrollBox, "sbWindow", "sbWinBox", 3, 4, 150, 80)
  look("winBox", "sbWinBox")
  step("createInBox", createScrollBox, "sbBox", "sbInnerBox", 5, 6, 60, 50)
  look("innerBox", "sbInnerBox")
  step("createMissingParent", createScrollBox, "sbNoSuch", "sbOrphan", 0, 0, 10, 10)
  look("orphan", "sbOrphan")
  step("createNoName", createScrollBox)
  step("createBadX", createScrollBox, "sbBad", "x", 0, 10, 10)
  step("createLabel", createLabel, "sbBox", "sbLabel", 7, 8, 40, 20, 1)
  look("label", "sbLabel")
  step("echo", echo, "sbLabel", "words")
  font("label", "sbLabel")
  step("createWinBoxLabel", createLabel, "sbWinBox", "sbWinBoxLabel", 1, 1, 40, 20, 1)
  step("echoWinBoxLabel", echo, "sbWinBoxLabel", "words")
  font("winBoxLabel", "sbWinBoxLabel")
  step("createInnerLabel", createLabel, "sbInnerBox", "sbInnerLabel", 2, 2, 20, 10, 1)
  look("innerLabel", "sbInnerLabel")
  step("createMini", createMiniConsole, "sbBox", "sbMini", 9, 10, 50, 40)
  look("mini", "sbMini")
  step("miniFontSize", getFontSize, "sbMini")
  step("createWinBoxMini", createMiniConsole, "sbWinBox", "sbWinBoxMini", 0, 0, 50, 40)
  step("winBoxMiniFontSize", getFontSize, "sbWinBoxMini")
  step("move", moveWindow, "sbBox", 11, 12)
  step("resize", resizeWindow, "sbBox", 120, 90)
  look("moved", "sbBox")
  step("raise", raiseWindow, "sbBox")
  step("lower", lowerWindow, "sbBox")
  step("hide", hideWindow, "sbBox")
  look("hidden", "sbBox")
  look("hiddenLabel", "sbLabel")
  look("hiddenMini", "sbMini")
  look("hiddenInnerBox", "sbInnerBox")
  look("hiddenInnerLabel", "sbInnerLabel")
  step("showLabelInHidden", showWindow, "sbLabel")
  look("labelInHidden", "sbLabel")
  step("show", showWindow, "sbBox")
  look("shown", "sbBox")
  look("shownLabel", "sbLabel")
  look("shownInnerLabel", "sbInnerLabel")
  step("hideWindow", hideWindow, "sbWindow")
  look("winBoxInHidden", "sbWinBox")
  look("winBoxLabelInHidden", "sbWinBoxLabel")
  step("showWindow", showWindow, "sbWindow")
  look("winBoxShown", "sbWinBox")
  -- A scroll box may share a user window's name: a label goes into the user window, a mini console into the box
  step("createClash", createScrollBox, "main", "sbWindow", 30, 30, 20, 20)
  step("createClashLabel", createLabel, "sbWindow", "sbClashLabel", 0, 0, 40, 20, 1)
  step("echoClashLabel", echo, "sbClashLabel", "words")
  font("clashLabel", "sbClashLabel")
  step("createClashMini", createMiniConsole, "sbWindow", "sbClashMini", 0, 0, 40, 20)
  step("hideClash", hideWindow, "sbWindow")
  step("clashHidden", windowVisible, "sbWindow")
  look("clashLabelHidden", "sbClashLabel")
  look("clashMiniHidden", "sbClashMini")
  step("showClash", showWindow, "sbWindow")
  step("deleteClash", deleteScrollBox, "sbWindow")
  step("clashDeleted", windowType, "sbWindow")
  step("bgImage", setBackgroundImage, "sbBox", "nothing.png")
  step("bgColor", setBackgroundColor, "sbBox", 1, 2, 3)
  events("beforeDelete")
  step("delete", deleteScrollBox, "sbBox")
  events("delete")
  look("deleted", "sbBox")
  look("deletedLabelAtOnce", "sbInnerLabel")
  look("deletedMiniAtOnce", "sbMini")
  look("deletedInnerAtOnce", "sbInnerBox")
  step("deleteAgain", deleteScrollBox, "sbBox")
  step("deleteEmpty", deleteScrollBox, "")
  step("deleteNumber", deleteScrollBox, 5)
  step("deleteNoArgs", deleteScrollBox)
  step("createAfterDelete", createScrollBox, "sbBox", 1, 1, 10, 10)
  look("recreated", "sbBox")
  step("deleteRecreated", deleteScrollBox, "sbBox")
  step("deleteMainBox", deleteScrollBox, "sbMainBox")
  step("deleteWinBox", deleteScrollBox, "sbWinBox")
  look("notYetGoneLabel", "sbLabel")
  look("notYetGoneWinBoxMini", "sbWinBoxMini")
  events("end")
  closeUserWindow("sbWindow")
  headlessScrollAnswers = table.concat(answers, "\n")
end)
headlessScroll = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessScrollAfter()
  local answers = {}
  for _, name in ipairs({"sbLabel", "sbInnerLabel", "sbMini", "sbInnerBox", "sbWinBoxLabel", "sbWinBoxMini", "sbBox", "sbClashMini", "sbClashLabel", "sbWindow"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessScroll"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("box:type=[1]scrollbox,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("createAgain=[2]false:scrollBox 'sbBox' already exists, moving/resizing 'sbBox'"),
                                  qsl("boxAgain:type=[1]scrollbox,geometry=[4]15:25:210:110,visible=[1]true"),
                                  qsl("createMain=[1]true"),
                                  qsl("mainBox:type=[1]scrollbox,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createInWindow=[1]true"),
                                  qsl("winBox:type=[1]scrollbox,geometry=[4]3:4:150:80,visible=[1]true"),
                                  qsl("createInBox=[1]true"),
                                  qsl("innerBox:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
                                  qsl("createMissingParent=[2]false:window 'sbNoSuch' not found"),
                                  qsl("orphan:type=[2]nil:'sbOrphan' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"sbOrphan\" not "
                                      "found,visible=[2]nil:window \"sbOrphan\" not found"),
                                  qsl("createNoName=raised:createScrollBox: bad argument #1 type (scrollBox name as string expected, got no value!)"),
                                  qsl("createBadX=raised:createScrollBox: bad argument #6 type (scrollBox height as number expected, got no value!)"),
                                  qsl("createLabel=[1]true"),
                                  qsl("label:type=[1]label,geometry=[4]7:8:40:20,visible=[1]true"),
                                  qsl("echo=[1]true"),
                                  qsl("label:font=main"),
                                  qsl("createWinBoxLabel=[1]true"),
                                  qsl("echoWinBoxLabel=[1]true"),
                                  qsl("winBoxLabel:font=sbWindow"),
                                  qsl("createInnerLabel=[1]true"),
                                  qsl("innerLabel:type=[1]label,geometry=[4]2:2:20:10,visible=[1]true"),
                                  qsl("createMini=[1]true"),
                                  qsl("mini:type=[1]miniconsole,geometry=[4]9:10:50:40,visible=[1]true"),
                                  qsl("miniFontSize=[1]12"),
                                  qsl("createWinBoxMini=[1]true"),
                                  qsl("winBoxMiniFontSize=[1]12"),
                                  qsl("move=[1]true"),
                                  qsl("resize=[1]true"),
                                  qsl("moved:type=[1]scrollbox,geometry=[4]11:12:120:90,visible=[1]true"),
                                  qsl("raise=[1]true"),
                                  qsl("lower=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("hidden:type=[1]scrollbox,geometry=[4]11:12:120:90,visible=[1]false"),
                                  qsl("hiddenLabel:type=[1]label,geometry=[4]7:8:40:20,visible=[1]false"),
                                  qsl("hiddenMini:type=[1]miniconsole,geometry=[4]9:10:50:40,visible=[1]false"),
                                  qsl("hiddenInnerBox:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]false"),
                                  qsl("hiddenInnerLabel:type=[1]label,geometry=[4]2:2:20:10,visible=[1]false"),
                                  qsl("showLabelInHidden=[1]true"),
                                  qsl("labelInHidden:type=[1]label,geometry=[4]7:8:40:20,visible=[1]false"),
                                  qsl("show=[1]true"),
                                  qsl("shown:type=[1]scrollbox,geometry=[4]11:12:120:90,visible=[1]true"),
                                  qsl("shownLabel:type=[1]label,geometry=[4]7:8:40:20,visible=[1]true"),
                                  qsl("shownInnerLabel:type=[1]label,geometry=[4]2:2:20:10,visible=[1]true"),
                                  qsl("hideWindow=[1]true"),
                                  qsl("winBoxInHidden:type=[1]scrollbox,geometry=[4]3:4:150:80,visible=[1]false"),
                                  qsl("winBoxLabelInHidden:type=[1]label,geometry=[4]1:1:40:20,visible=[1]false"),
                                  qsl("showWindow=[1]true"),
                                  qsl("winBoxShown:type=[1]scrollbox,geometry=[4]3:4:150:80,visible=[1]true"),
                                  qsl("createClash=[1]true"),
                                  qsl("createClashLabel=[1]true"),
                                  qsl("echoClashLabel=[1]true"),
                                  qsl("clashLabel:font=sbWindow"),
                                  qsl("createClashMini=[1]true"),
                                  qsl("hideClash=[1]true"),
                                  qsl("clashHidden=[1]false"),
                                  qsl("clashLabelHidden:type=[1]label,geometry=[4]0:0:40:20,visible=[1]false"),
                                  qsl("clashMiniHidden:type=[1]miniconsole,geometry=[4]0:0:40:20,visible=[1]true"),
                                  qsl("showClash=[1]true"),
                                  qsl("deleteClash=[1]true"),
                                  qsl("clashDeleted=[1]userwindow"),
                                  qsl("bgImage=[2]nil:console or label 'sbBox' not found, or 'nothing.png' could not be loaded as an image"),
                                  qsl("bgColor=[2]nil:'sbBox' is a scroll box, use setProfileStyleSheet() to set its background"),
                                  qsl("beforeDelete:events=sbWindow"),
                                  qsl("delete=[1]true"),
                                  qsl("delete:events=sbBox"),
                                  qsl("deleted:type=[2]nil:'sbBox' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"sbBox\" not "
                                      "found,visible=[2]nil:window \"sbBox\" not found"),
                                  qsl("deletedLabelAtOnce:type=[1]label,geometry=[4]2:2:20:10,visible=[1]true"),
                                  qsl("deletedMiniAtOnce:type=[1]miniconsole,geometry=[4]9:10:50:40,visible=[1]true"),
                                  qsl("deletedInnerAtOnce:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
                                  qsl("deleteAgain=[2]false:scrollbox name 'sbBox' not found"),
                                  qsl("deleteEmpty=[2]false:a scrollbox cannot have an empty string as its name"),
                                  qsl("deleteNumber=[2]false:scrollbox name '5' not found"),
                                  qsl("deleteNoArgs=raised:deleteScrollBox: bad argument #1 type (scrollbox name as string expected, got no value!)"),
                                  qsl("createAfterDelete=[1]true"),
                                  qsl("recreated:type=[1]scrollbox,geometry=[4]1:1:10:10,visible=[1]true"),
                                  qsl("deleteRecreated=[1]true"),
                                  qsl("deleteMainBox=[1]true"),
                                  qsl("deleteWinBox=[1]true"),
                                  qsl("notYetGoneLabel:type=[1]label,geometry=[4]7:8:40:20,visible=[1]true"),
                                  qsl("notYetGoneWinBoxMini:type=[1]miniconsole,geometry=[4]0:0:50:40,visible=[1]true"),
                                  qsl("end:events=sbBox,sbMainBox,sbWinBox")};
        QCOMPARE(luaGlobalString(host, "headlessScrollAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessScrollAfterAnswers = headlessScrollAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{qsl("sbLabel:nil"),
                                qsl("sbInnerLabel:nil"),
                                qsl("sbMini:nil"),
                                qsl("sbInnerBox:nil"),
                                qsl("sbWinBoxLabel:nil"),
                                qsl("sbWinBoxMini:nil"),
                                qsl("sbBox:nil"),
                                qsl("sbClashMini:nil"),
                                qsl("sbClashLabel:label"),
                                qsl("sbWindow:userwindow")};
        QCOMPARE(luaGlobalString(host, "headlessScrollAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making and deleting the scroll boxes created a widget.");
    }

    // A real scroll box, a command line and what the box holds are destroyed with the user window they are in
    void test_headlessUserWindowTakesItsScrollBoxes()
    {
        const QString hostname = qsl("Test-Headless-Host-User-Window-Scroll-Box");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
assert(openUserWindow("ubWindow", false, false, "f"))
assert(createScrollBox("ubWindow", "ubBox", 0, 0, 100, 100))
assert(createLabel("ubBox", "ubLabel", 0, 0, 10, 10, 1))
assert(createCommandLine("ubWindow", "ubLine", 0, 0, 10, 10))
assert(createCommandLine("ubBox", "ubBoxLine", 0, 0, 10, 10))
assert(createScrollBox("ubWindow", "ubShared", 0, 0, 100, 100))
hideWindow("ubShared")
assert(createCommandLine("ubShared", 3, 4, 40, 20))
assert(createScrollBox("ubLoneBox", 0, 0, 100, 100))
assert(createCommandLine("ubLoneBox", "ubLoneLine", 0, 0, 10, 10))
-- Weak keys, so a function only a command line still holds is collected
ubActions = setmetatable({}, {__mode = "k"})
for _, name in ipairs({"ubLine", "ubBoxLine", "ubLoneLine"}) do
  local action = function() end
  ubActions[action] = name
  setCmdLineAction(name, action)
end
assert(deleteMiniConsole("ubWindow"))
assert(deleteScrollBox("ubLoneBox"))
ubTypes = tostring(windowType("ubBox")) .. "," .. tostring(windowType("ubLabel")) .. "," .. tostring(windowType("ubLine")) .. "," .. tostring(windowType("ubBoxLine"))
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "ubTypes"), qsl("nil,nil,nil,nil"));
        QVERIFY(!host->windowRegistry().hasScrollBox(qsl("ubBox")));
        QVERIFY(!host->windowRegistry().hasLabel(qsl("ubLabel")));
        QVERIFY(!host->windowRegistry().hasCommandLine(qsl("ubLine")));
        QVERIFY(!host->windowRegistry().hasCommandLine(qsl("ubBoxLine")));

        // A main window command line of a scroll box's name is what is left of the name once the box is
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(
                qsl(R"lua(ubShared = tostring(windowType("ubShared")) .. ":" .. table.concat({getWindowGeometry("ubShared")}, ",") .. ":" .. tostring(windowVisible("ubShared")))lua"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        QCOMPARE(luaGlobalString(host, "ubShared"), qsl("commandline:3,4,40,20:true"));
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
collectgarbage("collect")
local kept = {}
for _, name in pairs(ubActions) do
  kept[#kept + 1] = name
end
table.sort(kept)
ubActionsKept = table.concat(kept, ",")
)lua"));
        QVERIFY2(ran, "The Lua chunk after collecting garbage did not run.");
        QCOMPARE(luaGlobalString(host, "ubActionsKept"), QString());
    }

    // As the GUI, where removing a hidden dock hides nothing, so a window elsewhere of the name of one it held answers
    // shown only once the deferred deletes report the name
    void test_headlessHiddenUserWindowReportsWhatItHeldOnItsDeferredDeletes()
    {
        const QString hostname = qsl("Test-Headless-Host-Hidden-User-Window-Deferred-Reports");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString answer = qsl(R"lua(uhAnswer = tostring(windowVisible("uhLine")) .. "," .. tostring(windowVisible("uhBox")))lua");
        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
assert(openUserWindow("uhWindow", false, false, "f"))
assert(createTextEdit("uhLine", 0, 300, 50, 50))
assert(createTextEdit("uhBox", 0, 360, 50, 50))
assert(createCommandLine("uhWindow", "uhLine", 0, 0, 50, 20))
assert(createScrollBox("uhWindow", "uhBox", 0, 0, 50, 50))
hideWindow("uhWindow")
assert(deleteMiniConsole("uhWindow"))
%1
)lua")
                                                                              .arg(answer));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "uhAnswer"), qsl("false,false"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(answer);
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QString drained = luaGlobalString(host, "uhAnswer");
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(drained, qsl("true,true"));
    }

    // As the GUI self-test profile answers, but for the frame and extra open-time resizes its window manager
    // adds: a user window tells scripts each new size it is shown at, one resized hidden or closed as it shows
    void test_headlessUserWindowResizeEventContract()
    {
        const QString hostname = qsl("Test-Headless-Host-User-Window-Resize");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onUwResize"));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
uwResize = "not run"
local ok, err = pcall(function()
  local events, during = {}, nil
  function onUwResize(event, width, height, name)
    events[#events + 1] = table.concat({type(width), width, type(height), height, type(name), name, tostring(windowVisible(name))}, ":")
    if during then
      local act = during
      during = nil
      act()
    end
  end
  -- The older hook for the same resizes, which a script may still define
  local legacy = handleWindowResizeEvent
  function handleWindowResizeEvent()
    events[#events + 1] = "hook"
  end
  local answers = {}
  local function step(label, f, ...)
    f(...)
    answers[#answers + 1] = label .. "=" .. table.concat(events, ";")
    events = {}
  end
  step("open", openUserWindow, "uwr", false, false, "f")
  step("resize", resizeWindow, "uwr", 300, 200)
  step("resizeSame", resizeWindow, "uwr", 300, 200)
  step("hide", hideWindow, "uwr")
  step("hiddenResize", resizeWindow, "uwr", 250, 150)
  step("show", showWindow, "uwr")
  step("hideAgain", hideWindow, "uwr")
  step("showUnchanged", showWindow, "uwr")
  step("reopenShown", openUserWindow, "uwr", false, false, "f")
  step("close", closeUserWindow, "uwr")
  step("closedResize", resizeWindow, "uwr", 280, 180)
  step("reopen", openUserWindow, "uwr", false, false, "f")
  step("negative", resizeWindow, "uwr", -5, -7)
  createMiniConsole("uwr", "uwrMini", 0, 0, 100, 100)
  step("innerMini", resizeWindow, "uwrMini", 50, 50)
  createMiniConsole("uwrMainMini", 0, 0, 100, 100)
  step("mainMini", resizeWindow, "uwrMainMini", 50, 50)
  -- As in the GUI, the window a show sends the resize to stays hidden through the handler, until a change reports its
  -- name, such as one to its own command line, or a layout load succeeds, and what it holds keeps its answer unless the
  -- handler makes, shows or hides it
  openUserWindow("uwrOther", false, false, "f")
  hideWindow("uwr")
  resizeWindow("uwr", 260, 170)
  events = {}
  during = function()
    hideWindow("uwrOther")
    showWindow("uwrOther")
    events[#events + 1] = "during:" .. tostring(windowVisible("uwr")) .. ":" .. tostring(windowVisible("uwrMini"))
  end
  step("showHidingOther", showWindow, "uwr")
  -- Qt applies a resize a handler makes to the window it is showing, but sends no event for it, then or at a later show
  hideWindow("uwr")
  resizeWindow("uwr", 240, 160)
  events = {}
  during = function()
    resizeWindow("uwr", 230, 140)
    events[#events + 1] = "during:" .. table.concat({getUserWindowSize("uwr")}, "x")
  end
  step("showResizingItself", showWindow, "uwr")
  step("reshowResized", function() hideWindow("uwr"); showWindow("uwr") end)
  -- One a handler opens in place of the window being shown is not the one being shown
  openUserWindow("uwrSwap", false, false, "f")
  hideWindow("uwrSwap")
  resizeWindow("uwrSwap", 210, 110)
  events = {}
  during = function()
    deleteMiniConsole("uwrSwap")
    openUserWindow("uwrSwap", false, false, "f")
    events[#events + 1] = "during:" .. tostring(windowVisible("uwrSwap"))
  end
  step("showReplacingItself", showWindow, "uwrSwap")
  handleWindowResizeEvent = legacy
  uwResizeAnswers = table.concat(answers, "\n")
end)
uwResize = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "uwResize"), qsl("ok"));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        const QStringList answers{
                qsl("open=hook;number:%1:number:%2:string:uwr:false").arg(opened.width()).arg(opened.height()),
                qsl("resize=hook;number:300:number:200:string:uwr:true"),
                qsl("resizeSame="),
                qsl("hide="),
                qsl("hiddenResize="),
                qsl("show=hook;number:250:number:150:string:uwr:false"),
                qsl("hideAgain="),
                qsl("showUnchanged="),
                qsl("reopenShown="),
                qsl("close="),
                qsl("closedResize="),
                qsl("reopen=hook;number:280:number:180:string:uwr:false"),
                qsl("negative=hook;number:0:number:0:string:uwr:true"),
                qsl("innerMini="),
                qsl("mainMini="),
                qsl("showHidingOther=hook;number:260:number:170:string:uwr:false;during:false:false"),
                qsl("showResizingItself=hook;number:240:number:160:string:uwr:false;during:230x140"),
                qsl("reshowResized="),
                qsl("showReplacingItself=hook;number:210:number:110:string:uwrSwap:false;hook;number:%1:number:%2:string:uwrSwap:false;during:true").arg(opened.width()).arg(opened.height())};
        QCOMPARE(luaGlobalString(host, "uwResizeAnswers").split(QLatin1Char('\n')), answers);
    }

    void test_headlessUserWindowResizeInItsShowHandler_data()
    {
        QTest::addColumn<QString>("during");
        QTest::addColumn<QString>("after");
        QTest::addColumn<QString>("expected");
        // Each answer: the events of the show and whether the window then shows, and the same for what follows it. The
        // window's pending resize is only cleared once the show's handlers return, so a show nested in them sends it again.
        const QString reshow = qsl("hideWindow('uws'); showWindow('uws')");
        QTest::newRow("hides and shows") << qsl("hideWindow('uws'); showWindow('uws')") << reshow << qsl("260x170:false;260x170:false|true||true");
        QTest::newRow("hides, resizes and shows") << qsl("hideWindow('uws'); resizeWindow('uws', 230, 140); showWindow('uws')") << reshow << qsl("260x170:false;230x140:false|true||true");
        QTest::newRow("hides, shows and resizes") << qsl("hideWindow('uws'); showWindow('uws'); resizeWindow('uws', 230, 140)") << reshow << qsl("260x170:false;260x170:false;230x140:true|true||true");
        QTest::newRow("resizes, hides and shows") << qsl("resizeWindow('uws', 230, 140); hideWindow('uws'); showWindow('uws')") << reshow << qsl("260x170:false;230x140:false|true||true");
        // A resize while hidden is the window's own, which its next show sends; one while still shown is dropped
        QTest::newRow("hides and resizes") << qsl("hideWindow('uws'); resizeWindow('uws', 230, 140)") << qsl("showWindow('uws')") << qsl("260x170:false|false|230x140:false|true");
        QTest::newRow("resizes and hides") << qsl("resizeWindow('uws', 230, 140); hideWindow('uws')") << qsl("showWindow('uws')") << qsl("260x170:false|false||true");
        // As QWidget::setVisible(true) on a widget not hidden, neither shows it again
        QTest::newRow("shows itself") << qsl("showWindow('uws')") << QString() << qsl("260x170:false|true||true");
        QTest::newRow("opens itself") << qsl("openUserWindow('uws', false, false, 'f')") << QString() << qsl("260x170:false|true||true");
    }

    // As the GUI self-test profile answers the same, but for the frame its sizes include
    void test_headlessUserWindowResizeInItsShowHandler()
    {
        QFETCH(const QString, during);
        QFETCH(const QString, after);
        QFETCH(const QString, expected);
        const QString hostname = qsl("Test-Headless-Host-User-Window-Show-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onUwShowHandler"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
uwShowHandler = "not run"
local events, during = {}, nil
function onUwShowHandler(_, width, height, name)
  if name == "uws" then
    events[#events + 1] = width .. "x" .. height .. ":" .. tostring(windowVisible(name))
    if during then
      local act = during
      during = nil
      act()
    end
  end
end
local ok, err = pcall(function()
  openUserWindow("uws", false, false, "f")
  resizeWindow("uws", 300, 200)
  hideWindow("uws")
  resizeWindow("uws", 260, 170)
  events = {}
  during = function() %1 end
  showWindow("uws")
  local shown = table.concat(events, ";") .. "|" .. tostring(windowVisible("uws"))
  events = {}
  %2
  uwShowHandler = shown .. "|" .. table.concat(events, ";") .. "|" .. tostring(windowVisible("uws"))
end)
if not ok then uwShowHandler = tostring(err) end
)lua")
                                                                           .arg(during, after)));
        const QString answer = luaGlobalString(host, "uwShowHandler");
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, expected);
    }

    void test_headlessUserWindowContentsInItsShowHandler_data()
    {
        QTest::addColumn<QString>("during");
        QTest::addColumn<QString>("expected");
        // Each answer: what the handler finds, what showWindow() returns and whether the window then shows. What is
        // inside the window answers as the GUI's isVisibleTo() once a change reaches it, and keeps its answer otherwise.
        QTest::newRow("another window shown") << qsl("hideWindow('uwcOther'); showWindow('uwcOther'); return windowVisible('uwcOld')") << qsl("false|true|true");
        QTest::newRow("new mini console, then another window shown") << qsl("createMiniConsole('uwc', 'uwcNew', 0, 0, 50, 50); hideWindow('uwcOther'); showWindow('uwcOther'); return "
                                                                            "tostring(windowVisible('uwcNew')) .. ',' .. tostring(windowVisible('uwcOld'))")
                                                                     << qsl("true,false|true|true");
        QTest::newRow("mini console hidden and shown") << qsl("hideWindow('uwcOld'); showWindow('uwcOld'); return windowVisible('uwcOld')") << qsl("true|true|true");
        // A show of what already shows changes nothing, so reports nothing
        QTest::newRow("mini console shown again") << qsl("showWindow('uwcOld'); return windowVisible('uwcOld')") << qsl("false|true|true");
        QTest::newRow("window shown again") << qsl("showWindow('uwc'); return windowVisible('uwcOld')") << qsl("false|true|true");
        QTest::newRow("window opened again") << qsl("openUserWindow('uwc', false, false, 'f'); return windowVisible('uwcOld')") << qsl("false|true|true");
        QTest::newRow("label shown again") << qsl("showWindow('uwcOldLabel'); return windowVisible('uwcOldLabel')") << qsl("false|true|true");
        QTest::newRow("scroll box shown again") << qsl("showWindow('uwcOpenBox'); return windowVisible('uwcOpenBoxLabel')") << qsl("false|true|true");
        QTest::newRow("command line shown again") << qsl("showWindow('uwcOldCmd'); return windowVisible('uwcOldCmd')") << qsl("false|true|true");
        QTest::newRow("command line enabled again") << qsl("enableCommandLine('uwcOldCmd'); return windowVisible('uwcOldCmd')") << qsl("false|true|true");
        QTest::newRow("hidden scroll box shown") << qsl("showWindow('uwcBox'); return tostring(windowVisible('uwcBoxLabel')) .. ',' .. tostring(windowVisible('uwcOld'))")
                                                 << qsl("true,false|true|true");
        QTest::newRow("new label, then the window hidden") << qsl("createLabel('uwc', 'uwcLabel', 0, 0, 20, 20, 1); hideWindow('uwc'); return windowVisible('uwcLabel')") << qsl("false|true|false");
        QTest::newRow("window deleted") << qsl("deleteMiniConsole('uwc'); return 'deleted'") << qsl("deleted|false|nil");
        QTest::newRow("window deleted and reopened") << qsl("deleteMiniConsole('uwc'); openUserWindow('uwc', false, false, 'f'); return 'reopened'") << qsl("reopened|true|true");
        // As the GUI's reportDockGeometry() once a load succeeds, here from a layout without the window
        QTest::newRow("layout loaded, then a scroll box shown") << qsl(
                "assert(loadWindowLayout()); showWindow('uwcBox'); return tostring(windowVisible('uwc')) .. ',' .. tostring(windowVisible('uwcOld'))")
                                                                << qsl("true,false|true|true");
        QTest::newRow("opened with the layout") << qsl("openUserWindow('uwc', true, false, 'f'); return tostring(windowVisible('uwc')) .. ',' .. tostring(windowVisible('uwcOld'))")
                                                << qsl("true,false|true|true");
    }

    // As the GUI self-test profile answers the same
    void test_headlessUserWindowContentsInItsShowHandler()
    {
        QFETCH(const QString, during);
        QFETCH(const QString, expected);
        const QString hostname = qsl("Test-Headless-Host-User-Window-Contents-Show-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onUwcShowHandler"));
        QFile::remove(headlessLayoutPath());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
uwcShowHandler = "not run"
local during, found = nil, "not run"
function onUwcShowHandler(_, _, _, name)
  if name == "uwc" and during then
    local act = during
    during = nil
    found = tostring(act())
  end
end
local ok, err = pcall(function()
  assert(saveWindowLayout())
  openUserWindow("uwc", false, false, "f")
  openUserWindow("uwcOther", false, false, "f")
  createMiniConsole("uwc", "uwcOld", 0, 0, 50, 50)
  createLabel("uwc", "uwcOldLabel", 0, 0, 20, 20, 1)
  createCommandLine("uwc", "uwcOldCmd", 0, 60, 100, 20)
  createScrollBox("uwc", "uwcOpenBox", 0, 0, 80, 80)
  createLabel("uwcOpenBox", "uwcOpenBoxLabel", 0, 0, 20, 20, 1)
  createScrollBox("uwc", "uwcBox", 0, 0, 80, 80)
  createLabel("uwcBox", "uwcBoxLabel", 0, 0, 20, 20, 1)
  hideWindow("uwcBox")
  hideWindow("uwc")
  resizeWindow("uwc", 260, 170)
  during = function() %1 end
  local shown = showWindow("uwc")
  uwcShowHandler = found .. "|" .. tostring(shown) .. "|" .. tostring(windowVisible("uwc"))
end)
if not ok then uwcShowHandler = tostring(err) end
)lua")
                                                                           .arg(during)));
        const QString answer = luaGlobalString(host, "uwcShowHandler");
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, expected);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As the GUI self-test profile answers, but for the frame and positions its window manager chooses
    void test_headlessWindowLayoutRoundTripContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlResult = "not run"
local ok, err = pcall(function()
  local events = {}
  function onWlResize(_, width, height, name)
    events[#events + 1] = table.concat({width, height, name}, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    local results = {f(...)}
    for i = 1, #results do results[i] = tostring(results[i]) end
    answers[#answers + 1] = label .. "=" .. table.concat(results, ":") .. "|" .. table.concat(events, ";")
    events = {}
  end
  step("loadNone", loadWindowLayout)
  step("openA", openUserWindow, "wlA", false, false, "f")
  step("openB", openUserWindow, "wlB", false, false, "f")
  step("placeA", function() resizeWindow("wlA", 300, 200); moveWindow("wlA", 40, 50) end)
  step("placeB", function() resizeWindow("wlB", 320, 220); hideWindow("wlB") end)
  step("save", saveWindowLayout)
  step("saveAgain", saveWindowLayout)
  step("changeA", function() resizeWindow("wlA", 400, 330); moveWindow("wlA", 70, 80) end)
  step("changeB", function() resizeWindow("wlB", 410, 340); showWindow("wlB") end)
  step("openC", function() openUserWindow("wlC", false, false, "f"); resizeWindow("wlC", 150, 100) end)
  step("load", loadWindowLayout)
  step("geometryA", getWindowGeometry, "wlA")
  step("sizeA", getUserWindowSize, "wlA")
  step("geometryB", getWindowGeometry, "wlB")
  step("visibleB", windowVisible, "wlB")
  step("geometryC", getWindowGeometry, "wlC")
  step("visibleC", windowVisible, "wlC")
  step("loadAgain", loadWindowLayout)
  -- closed when saved, resized while closed, then reopened with the layout
  step("openD", function() openUserWindow("wlD", false, false, "f"); resizeWindow("wlD", 222, 111); closeUserWindow("wlD") end)
  step("saveClosed", saveWindowLayout)
  step("resizeClosedD", resizeWindow, "wlD", 333, 144)
  step("reopenD", openUserWindow, "wlD", true, false, "f")
  step("geometryD", getWindowGeometry, "wlD")
  step("visibleD", windowVisible, "wlD")
  -- a second open with the layout does not load it again
  step("resizeD", resizeWindow, "wlD", 250, 120)
  step("reopenDAgain", openUserWindow, "wlD", true, false, "f")
  step("geometryDAgain", getWindowGeometry, "wlD")
  -- not in the saved layout
  step("openE", openUserWindow, "wlE", true, false, "f")
  step("geometryE", getWindowGeometry, "wlE")
  step("geometryAAfterE", getWindowGeometry, "wlA")
  wlAnswers = table.concat(answers, "\n")
end)
wlResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlResult"), qsl("ok"));
        QVERIFY2(QFile::exists(layoutFile), "saveWindowLayout() wrote no layout file.");
        const QSize opened = host->mainWindowSize().value_or(QSize());
        const QString openedSize = qsl("%1:%2").arg(opened.width()).arg(opened.height());
        const QStringList answers{qsl("loadNone=false|"),
                                  qsl("openA=true|%1:wlA").arg(openedSize),
                                  qsl("openB=true|%1:wlB").arg(openedSize),
                                  qsl("placeA=|300:200:wlA"),
                                  qsl("placeB=|320:220:wlB"),
                                  qsl("save=true|"),
                                  qsl("saveAgain=true|"),
                                  qsl("changeA=|400:330:wlA"),
                                  qsl("changeB=|410:340:wlB"),
                                  qsl("openC=|%1:wlC;150:100:wlC").arg(openedSize),
                                  qsl("load=true|300:200:wlA"),
                                  qsl("geometryA=40:50:300:200|"),
                                  qsl("sizeA=300:200|"),
                                  qsl("geometryB=0:0:320:220|"),
                                  qsl("visibleB=false|"),
                                  qsl("geometryC=0:0:150:100|"),
                                  qsl("visibleC=true|"),
                                  qsl("loadAgain=true|"),
                                  qsl("openD=|%1:wlD;222:111:wlD").arg(openedSize),
                                  qsl("saveClosed=true|"),
                                  qsl("resizeClosedD=true|"),
                                  qsl("reopenD=true|"),
                                  qsl("geometryD=0:0:222:111|"),
                                  qsl("visibleD=true|"),
                                  qsl("resizeD=true|250:120:wlD"),
                                  qsl("reopenDAgain=true|"),
                                  qsl("geometryDAgain=0:0:250:120|"),
                                  qsl("openE=true|%1:wlE").arg(openedSize),
                                  qsl("geometryE=0:0:%1|").arg(openedSize),
                                  qsl("geometryAAfterE=40:50:300:200|")};
        QCOMPARE(luaGlobalString(host, "wlAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY(QFile::remove(layoutFile));
    }

    // With nothing held of a closed profile, a load from a file of a later version, or a damaged one, answers false and
    // places nothing
    void test_headlessWindowLayoutRefusesDamagedFile()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Damaged");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlDamagedResult = "not run"
local ok, err = pcall(function()
  local events = {}
  function onWlResize(_, width, height, name)
    events[#events + 1] = table.concat({width, height, name}, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    local results = {f(...)}
    for i = 1, #results do results[i] = tostring(results[i]) end
    answers[#answers + 1] = label .. "=" .. table.concat(results, ":") .. "|" .. table.concat(events, ";")
    events = {}
  end
  local path = getMudletHomeDir():match("^(.*)/profiles/[^/]*$") .. "/windowLayoutHeadless.dat"
  local function writeBytes(bytes)
    local handle = assert(io.open(path, "wb"))
    handle:write(bytes)
    handle:close()
  end
  step("open", function() openUserWindow("wlA", false, false, "f"); resizeWindow("wlA", 300, 200) end)
  -- a save lands at this path, so the refusals below are of this file and not of a missing one
  step("saveFirst", function()
    assert(saveWindowLayout())
    local handle = assert(io.open(path, "rb"))
    local bytes = handle:read("*a")
    handle:close()
    return #bytes > 0
  end)
  step("otherVersion", function() writeBytes("MWLH\0\0\0\2"); return loadWindowLayout() end)
  step("foreign", function() writeBytes("junk"); return loadWindowLayout() end)
  step("truncated", function() writeBytes("MWLH\0\0\0\1\0\0"); return loadWindowLayout() end)
  step("geometry", getWindowGeometry, "wlA")
  step("saveOver", saveWindowLayout)
  step("resize", resizeWindow, "wlA", 400, 330)
  step("loadOver", loadWindowLayout)
  wlDamagedAnswers = table.concat(answers, "\n")
end)
wlDamagedResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlDamagedResult"), qsl("ok"));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        const QStringList answers{qsl("open=|%1:%2:wlA;300:200:wlA").arg(opened.width()).arg(opened.height()),
                                  qsl("saveFirst=true|"),
                                  qsl("otherVersion=false|"),
                                  qsl("foreign=false|"),
                                  qsl("truncated=false|"),
                                  qsl("geometry=0:0:300:200|"),
                                  qsl("saveOver=true|"),
                                  qsl("resize=true|400:330:wlA"),
                                  qsl("loadOver=true|300:200:wlA")};
        QCOMPARE(luaGlobalString(host, "wlDamagedAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY(QFile::remove(layoutFile));
    }

    // Short, foreign or pre-first-version files are damaged and replaced by a save; one that cannot be opened, such as a
    // directory at its path, is kept, as a later version's is
    void test_headlessWindowLayoutReplacesDamagedFileButNotUnopenable()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Short");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFilePath = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFilePath);
        const QString saveAndLoad = qsl(R"lua(
resizeWindow("wlA", 300, 200)
local saved = saveWindowLayout()
resizeWindow("wlA", 400, 330)
wlShort = tostring(saved) .. "|" .. tostring(loadWindowLayout()) .. "|" .. table.concat({getWindowGeometry("wlA")}, ":")
)lua");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(openUserWindow("wlA", false, false, "f"))lua")));

        QList<QByteArray> damaged;
        for (const int size : {0, 1, 2, 3, 4, 8}) {
            damaged.append(QByteArray(size, '\0'));
        }
        damaged << QByteArray("junk") << QByteArray::fromHex("4d574c48") << QByteArray::fromHex("4d574c4800000000") << QByteArray::fromHex("484c574d00000001")
                << QByteArray::fromHex("484c574d00000002");
        // Whole user windows, then map widgets cut short, so a save must drop what was read before the damage
        QByteArray truncated;
        {
            QDataStream ofs(&truncated, QIODevice::WriteOnly);
            ofs.setVersion(QDataStream::Qt_5_12);
            ofs << quint32(0x4d574c48) << quint32(1) << QMap<QString, std::pair<QRect, bool>>{{hostname + qsl("/wlZ"), {QRect(1, 2, 3, 4), true}}} << quint32(1);
        }
        damaged << truncated;
        for (const QByteArray& bytes : damaged) {
            QVERIFY(writeHeadlessLayout(bytes));
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("windowLayoutHeadless\\.dat\" is damaged.* - this save replaces it$")));
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(saveAndLoad));
            QCOMPARE(luaGlobalString(host, "wlShort"), qsl("true|true|0:0:300:200"));
            QVERIFY2(!savedUserWindows().contains(hostname + qsl("/wlZ")), "A save kept a window read from a damaged file.");
        }

        QVERIFY(QFile::remove(layoutFilePath));
        QVERIFY(QDir().mkpath(layoutFilePath));
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(qsl("error opening window layout file: .*windowLayoutHeadless\\.dat\" .* - this save does not replace it: delete or rename it to save one$")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("error opening window layout file: .*windowLayoutHeadless\\.dat\" .* until it is deleted or renamed$")));
        // Once each for the save and the load
        QTest::failOnWarning(QRegularExpression(qsl("window layout file")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(saveAndLoad));
        QCOMPARE(luaGlobalString(host, "wlShort"), qsl("false|false|0:0:400:330"));
        QVERIFY(QFileInfo(layoutFilePath).isDir());
        QVERIFY(QDir().rmdir(layoutFilePath));
        HostManager::self()->deleteHost(hostname);
        QFile::remove(layoutFilePath);
    }

    // Two profiles' windows of one name are placed apart, as the GUI keys a floating dock's geometry by profile
    void test_headlessWindowLayoutKeepsProfilesApart()
    {
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const QString chunk = qsl(R"lua(
wlApartResult = "not run"
local ok, err = pcall(function()
  wlEvents = {}
  function onWlResize(_, width, height, name)
    wlEvents[#wlEvents + 1] = table.concat({width, height, name}, ":")
  end
  openUserWindow("wlA", false, false, "f")
  resizeWindow("wlA", WIDTH, HEIGHT)
  moveWindow("wlA", X, Y)
  if HIDDEN then hideWindow("wlA") end
  wlEvents = {}
end)
wlApartResult = ok and "ok" or tostring(err)
)lua");
        const QString placed = qsl(R"lua(
wlPlaced = table.concat({getWindowGeometry("wlA")}, ":") .. ":" .. tostring(windowVisible("wlA")) .. "|" .. table.concat(wlEvents, ";")
wlEvents = {}
)lua");
        std::array<Host*, 2> hosts{};
        const std::array<QString, 2> hostnames{qsl("Test-Headless-Host-Window-Layout-One"), qsl("Test-Headless-Host-Window-Layout-Two")};
        const std::array<QString, 2> placements{qsl("WIDTH, HEIGHT, X, Y, HIDDEN = 300, 200, 10, 20, false"), qsl("WIDTH, HEIGHT, X, Y, HIDDEN = 150, 100, 30, 40, true")};
        for (size_t i = 0; i < hosts.size(); ++i) {
            QVERIFY2(HostManager::self()->addHost(hostnames[i], QString(), QString(), QString()), "Could not create a profile with no main window.");
            hosts[i] = HostManager::self()->getHost(hostnames[i]);
            QVERIFY2(hosts[i], "The profile is not in the pool.");
            hosts[i]->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
            QVERIFY(hosts[i]->getLuaInterpreter()->compileAndExecuteScript(placements[i] + chunk));
            QCOMPARE(luaGlobalString(hosts[i], "wlApartResult"), qsl("ok"));
        }
        QVERIFY(hosts[0]->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(saveWindowLayout())")));
        QVERIFY(hosts[0]->getLuaInterpreter()->compileAndExecuteScript(qsl("resizeWindow('wlA', 400, 330); wlEvents = {}")));
        QVERIFY(hosts[1]->getLuaInterpreter()->compileAndExecuteScript(qsl("showWindow('wlA'); resizeWindow('wlA', 500, 400); wlEvents = {}")));
        QVERIFY(hosts[1]->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(loadWindowLayout())")));
        QVERIFY(hosts[0]->getLuaInterpreter()->compileAndExecuteScript(placed));
        QVERIFY(hosts[1]->getLuaInterpreter()->compileAndExecuteScript(placed));
        QCOMPARE(luaGlobalString(hosts[0], "wlPlaced"), qsl("10:20:300:200:true|300:200:wlA"));
        QCOMPARE(luaGlobalString(hosts[1], "wlPlaced"), qsl("30:40:150:100:false|"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // As in the GUI's restoreState(), resize handlers run mid-load; a load from one is refused, and so is a save, as it
    // would write windows not yet placed
    void test_headlessWindowLayoutLoadRefusesReentry()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Reentry");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlReentryResult = "not run"
local ok, err = pcall(function()
  local inLoad = {}
  local armed = false
  function onWlResize(_, width, height, name)
    if armed then
      armed = false
      local saved, saveError = saveWindowLayout()
      inLoad[#inLoad + 1] = "save=" .. tostring(saved) .. ":" .. tostring(saveError) .. ",load=" .. tostring(loadWindowLayout())
    end
  end
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  assert(saveWindowLayout())
  resizeWindow("wlA", 400, 330)
  resizeWindow("wlB", 410, 340)
  local function sizes()
    return table.concat({getWindowGeometry("wlA")}, ":") .. "," .. table.concat({getWindowGeometry("wlB")}, ":")
  end
  armed = true
  assert(loadWindowLayout())
  local answers = {table.concat(inLoad, ";"), sizes()}
  -- the handler's save, had it happened, writes the window this load had yet to reach at its old size
  resizeWindow("wlA", 500, 400)
  resizeWindow("wlB", 510, 410)
  assert(loadWindowLayout())
  answers[#answers + 1] = sizes()
  wlReentryAnswers = table.concat(answers, "|")
end)
wlReentryResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlReentryResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "wlReentryAnswers"), qsl("save=nil:cannot save the window layout while it is being loaded,load=false|0:0:300:200,0:0:320:220|0:0:300:200,0:0:320:220"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // As TMainConsole::closeEvent() asks for a save after sysExitEvent, but every close writes, not only those until one
    // in the session has saved
    void test_headlessWindowLayoutSavedAsProfileCloses()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Close");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 123, 45)
moveWindow("wlA", 7, 8)
hideWindow("wlA")
function onWlExit() resizeWindow("wlA", 130, 60) end
registerAnonymousEventHandler("sysExitEvent", "onWlExit")
)lua")));
        QVERIFY(!QFile::exists(layoutFile));
        HostManager::self()->deleteHost(hostname);
        QVERIFY2(QFile::exists(layoutFile), "Closing the profile saved no layout.");

        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", true, false, "f")
wlReopened = table.concat({getWindowGeometry("wlA")}, ":") .. ":" .. tostring(windowVisible("wlA"))
)lua")));
        QCOMPARE(luaGlobalString(host, "wlReopened"), qsl("7:8:130:60:true"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // A save keeps a closed profile's windows, else a later save would undo the one its close made
    void test_headlessWindowLayoutKeepsClosedProfilesPlacements()
    {
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const std::array<QString, 2> hostnames{qsl("Test-Headless-Host-Window-Layout-Closed-One"), qsl("Test-Headless-Host-Window-Layout-Closed-Two")};
        const std::array<QString, 2> sizes{qsl("101, 51"), qsl("202, 52")};
        for (size_t i = 0; i < hostnames.size(); ++i) {
            QVERIFY2(HostManager::self()->addHost(hostnames[i], QString(), QString(), QString()), "Could not create a profile with no main window.");
            Host* host = HostManager::self()->getHost(hostnames[i]);
            QVERIFY2(host, "The profile is not in the pool.");
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', %1)").arg(sizes[i])));
        }
        for (const QString& hostname : hostnames) {
            HostManager::self()->deleteHost(hostname);
        }
        QVERIFY2(QFile::exists(layoutFile), "Closing the profiles saved no layout.");
        const std::array<QString, 2> reopened{qsl("0:0:101:51"), qsl("0:0:202:52")};
        for (size_t i = 0; i < hostnames.size(); ++i) {
            QVERIFY2(HostManager::self()->addHost(hostnames[i], QString(), QString(), QString()), "Could not create the profile again.");
            Host* host = HostManager::self()->getHost(hostnames[i]);
            QVERIFY2(host, "The profile is not in the pool.");
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', true, false, 'f'); wlReopened = table.concat({getWindowGeometry('wlA')}, ':')")));
            QCOMPARE(luaGlobalString(host, "wlReopened"), reopened[i]);
        }
        QVERIFY(QFile::remove(layoutFile));
    }

    // A window a resize handler opens with the layout during a load has its own load refused, so the
    // running load places it
    void test_headlessWindowLayoutPlacesWindowOpenedDuringLoad()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Opened-In-Load");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlOpenedResult = "not run"
local ok, err = pcall(function()
  local armed = false
  function onWlResize(_, _, _, name)
    if armed and name == "wlA" then
      armed = false
      openUserWindow("wlB", true, false, "f")
    end
  end
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  assert(saveWindowLayout())
  assert(deleteMiniConsole("wlB"))
  assert(getWindowGeometry("wlB") == nil, "wlB is still there")
  resizeWindow("wlA", 400, 330)
  armed = true
  assert(loadWindowLayout())
  assert(not armed, "the handler did not open wlB")
  wlOpenedAnswers = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getWindowGeometry("wlB")}, ":") .. ":" .. tostring(windowVisible("wlB"))
end)
wlOpenedResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlOpenedResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "wlOpenedAnswers"), qsl("0:0:300:200|0:0:320:220:true"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // A window a resize handler opens with the layout during a load stays shown though saved hidden: in the GUI it
    // is not a dock QMainWindow::restoreState() began with, so only restoreFloatingDockGeometries() after it places it
    void test_headlessWindowLayoutKeepsWindowOpenedDuringLoadShown()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Opened-In-Load-Shown");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlShownResult = "not run"
local ok, err = pcall(function()
  local armed = false
  function onWlResize(_, _, _, name)
    if armed and name == "wlA" then
      armed = false
      openUserWindow("wlB", true, false, "f")
    end
  end
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  assert(hideWindow("wlB"))
  assert(saveWindowLayout())
  assert(deleteMiniConsole("wlB"))
  resizeWindow("wlA", 400, 330)
  armed = true
  assert(loadWindowLayout())
  assert(not armed, "the handler did not open wlB")
  wlShownAnswers = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getWindowGeometry("wlB")}, ":") .. ":" .. tostring(windowVisible("wlB"))
end)
wlShownResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlShownResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "wlShownAnswers"), qsl("0:0:300:200|0:0:320:220:true"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessUserWindowDeletedByFontChangeHandler()
    {
        const QString hostname = qsl("Test-Headless-Host-User-Window-Deleted-By-Font-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
fontGoneResult = "not run"
local ok, err = pcall(function()
  local function pack(...)
    local parts = {}
    for i = 1, select("#", ...) do parts[#parts + 1] = tostring((select(i, ...))) end
    return "[" .. select("#", ...) .. "]" .. table.concat(parts, ":")
  end
  local answers = {}
  local target, replace, deleted
  function onFontGone(_, window)
    if window == target then
      target = nil
      deleted = pack(deleteMiniConsole(window))
      if replace then
        -- the call must neither load the layout for this one nor show it
        deleted = deleted .. "+" .. pack(openUserWindow(window, false, false, "f")) .. "+" .. pack(hideWindow(window))
      end
    end
  end
  registerAnonymousEventHandler("sysFontChangeEvent", "onFontGone")
  -- a window so deleted does not load the layout, which would put this one back
  openUserWindow("fontOther", false, false, "f")
  resizeWindow("fontOther", 300, 200)
  assert(saveWindowLayout())
  for _, case in ipairs({{"fontGonefalse", false, false}, {"fontGonetrue", true, false}, {"fontReplaced", true, true}}) do
    local name, loadLayout = case[1], case[2]
    target, replace, deleted = name, case[3], "not fired"
    resizeWindow("fontOther", 400, 330)
    local opened = pack(openUserWindow(name, loadLayout, false, "f"))
    answers[#answers + 1] = table.concat({deleted, opened, pack(windowType(name)), pack(windowVisible(name)), pack(getWindowGeometry("fontOther"))}, "|")
  end
  target = nil
  -- nothing of a deleted one is left behind to stop it opening again
  local reopened = {}
  for _, loadLayout in ipairs({false, true}) do
    local reopenName = "fontGone" .. tostring(loadLayout)
    openUserWindow(reopenName, true, false, "f")
    reopened[#reopened + 1] = pack(windowType(reopenName))
  end
  answers[#answers + 1] = table.concat(reopened, "|")
  fontGoneAnswers = table.concat(answers, "\n")
end)
fontGoneResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "fontGoneResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "fontGoneAnswers"),
                 qsl("[1]true|[2]nil:userwindow 'fontGonefalse' was deleted while it was being opened|[2]nil:'fontGonefalse' is not a known label, any type of console, command "
                     "line, text edit, nor scroll box|[2]nil:window \"fontGonefalse\" not found|[4]0:0:400:330\n"
                     "[1]true|[2]nil:userwindow 'fontGonetrue' was deleted while it was being opened|[2]nil:'fontGonetrue' is not a known label, any type of console, command "
                     "line, text edit, nor scroll box|[2]nil:window \"fontGonetrue\" not found|[4]0:0:400:330\n"
                     "[1]true+[1]true+[1]true|[2]nil:userwindow 'fontReplaced' was deleted while it was being opened|[1]userwindow|[1]false|[4]0:0:400:330\n"
                     "[1]userwindow|[1]userwindow"));
        QFile::remove(layoutFile);
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessUserWindowReplacedByResizeHandler()
    {
        const QString hostname = qsl("Test-Headless-Host-User-Window-Replaced-By-Resize-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
resizeReplacedResult = "not run"
local ok, err = pcall(function()
  local function pack(...)
    local parts = {}
    for i = 1, select("#", ...) do parts[#parts + 1] = tostring((select(i, ...))) end
    return "[" .. select("#", ...) .. "]" .. table.concat(parts, ":")
  end
  local answers = {}
  local trigger, target, replaced
  function onResizeReplace(_, _, _, window)
    if window == trigger then
      trigger = nil
      replaced = pack(deleteMiniConsole(target)) .. "+" .. pack(openUserWindow(target, false, false, "f")) .. "+" .. pack(hideWindow(target))
    end
  end
  registerAnonymousEventHandler("sysUserWindowResizeEvent", "onResizeReplace")
  openUserWindow("resizeOther", false, false, "f")
  resizeWindow("resizeOther", 300, 200)
  assert(saveWindowLayout())
  -- replaced as the load resizes another window, then as its own show sends it its size
  for _, case in ipairs({{"resizeInLoad", true, "resizeOther"}, {"resizeInShow", false, "resizeInShow"}}) do
    local name, loadLayout = case[1], case[2]
    resizeWindow("resizeOther", 400, 330)
    trigger, target, replaced = case[3], name, "not fired"
    local opened = pack(openUserWindow(name, loadLayout, false, "f"))
    answers[#answers + 1] = table.concat({replaced, opened, pack(windowType(name)), pack(windowVisible(name)), pack(getWindowGeometry("resizeOther"))}, "|")
  end
  trigger = nil
  resizeReplacedAnswers = table.concat(answers, "\n")
end)
resizeReplacedResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "resizeReplacedResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "resizeReplacedAnswers"),
                 qsl("[1]true+[1]true+[1]true|[2]nil:userwindow 'resizeInLoad' was deleted while it was being opened|[1]userwindow|[1]false|[4]0:0:300:200\n"
                     "[1]true+[1]true+[1]true|[2]nil:userwindow 'resizeInShow' was deleted while it was being opened|[1]userwindow|[1]false|[4]0:0:400:330"));
        QFile::remove(layoutFile);
    }

    // A placed window a later handler reopens with the layout is new and placed again: in the GUI mIsLoadingLayout
    // refuses the reopen's load, and restoreFloatingDockGeometries() after restoreState() places it
    void test_headlessWindowLayoutPlacesWindowReopenedDuringLoad()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Reopened-In-Load");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlReopenedResult = "not run"
local ok, err = pcall(function()
  -- which window the load places first is not pinned, so the handler reopens whichever was
  local armed, placedFirst, done = false, nil, false
  function onWlResize(_, _, _, name)
    if not armed or done then return end
    if not placedFirst then
      placedFirst = name
    else
      done = true
      assert(deleteMiniConsole(placedFirst))
      openUserWindow(placedFirst, true, false, "f")
    end
  end
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  assert(saveWindowLayout())
  resizeWindow("wlA", 400, 330)
  resizeWindow("wlB", 410, 340)
  armed = true
  assert(loadWindowLayout())
  assert(done, "the handler did not reopen " .. tostring(placedFirst))
  wlReopenedAnswers = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getWindowGeometry("wlB")}, ":")
end)
wlReopenedResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlReopenedResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "wlReopenedAnswers"), qsl("0:0:300:200|0:0:320:220"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // The bytes are written out by hand so the format, not just a round trip, is pinned. A file of a later
    // version is refused by a save too, which would otherwise drop every placement it holds.
    void test_headlessWindowLayoutFileFormat()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Format");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFilePath = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        auto bigEndian = [](QByteArray& bytes, const quint32 value) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                bytes.append(static_cast<char>((value >> shift) & 0xff));
            }
        };
        // A one-entry map: the key's length in bytes and its UTF-16, a QRect as left, top, right and bottom, then shown
        auto placement = [&](QByteArray& bytes, const QString& key, const std::array<quint32, 4>& edges, const bool shown) {
            bigEndian(bytes, 1);
            bigEndian(bytes, key.size() * 2);
            for (const QChar character : key) {
                bytes.append(static_cast<char>(character.unicode() >> 8));
                bytes.append(static_cast<char>(character.unicode() & 0xff));
            }
            for (const quint32 edge : edges) {
                bigEndian(bytes, edge);
            }
            bytes.append(shown ? '\x01' : '\x00');
        };
        auto fixture = [&](const quint32 version) {
            QByteArray bytes("MWLH");
            bigEndian(bytes, version);
            // The user windows', then the map widgets': QRect(7, 8, 123, 45) shown, and QRect(20, 30, 200, 100) hidden
            placement(bytes, hostname + qsl("/wlA"), {7, 8, 129, 52}, true);
            placement(bytes, hostname, {20, 30, 219, 129}, false);
            return bytes;
        };
        auto writeLayoutFile = writeHeadlessLayout;
        auto readLayoutFile = [&layoutFilePath]() {
            QFile layoutFile(layoutFilePath);
            return layoutFile.open(QIODevice::ReadOnly) ? layoutFile.readAll() : QByteArray();
        };

        QVERIFY(writeLayoutFile(fixture(1)));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", true, false, "f")
wlFormatOne = table.concat({getWindowGeometry("wlA")}, ":") .. ":" .. tostring(windowVisible("wlA"))
)lua")));
        QCOMPARE(luaGlobalString(host, "wlFormatOne"), qsl("7:8:123:45:true"));
        // The first map widget takes its saved place, and is shown though saved hidden, as in the GUI
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(wlFormatMap = tostring(openMapWidget()) .. "|" .. table.concat({getMapWidgetGeometry()}, ":"))lua")));
        QCOMPARE(luaGlobalString(host, "wlFormatMap"), qsl("true|20:30:200:100"));

        QVERIFY(writeLayoutFile(fixture(2)));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
assert(deleteMiniConsole("wlA"))
openUserWindow("wlA", true, false, "f")
wlFormatTwo = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. tostring(loadWindowLayout()) .. "|" .. tostring(saveWindowLayout())
)lua")));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlFormatTwo"), qsl("0:0:%1:%2|false|false").arg(opened.width()).arg(opened.height()));
        QCOMPARE(readLayoutFile(), fixture(2));
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(readLayoutFile(), fixture(2));
        // Its refused saves leave its windows held, for the next save that succeeds, which is this one rather than the next test's
        QVERIFY(QFile::remove(layoutFilePath));
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        QVERIFY(QFile::remove(layoutFilePath));
    }

    // As the GUI, where a deleted dock stays in the saved layout where it was when deleted, even if that placement was never saved
    void test_headlessWindowLayoutKeepsDeletedWindowsPlacement()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Deleted");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(saveWindowLayout())
resizeWindow("wlA", 260, 170)
assert(deleteMiniConsole("wlA"))
assert(saveWindowLayout())
openUserWindow("wlA", true, false, "f")
wlDeletedReopened = table.concat({getWindowGeometry("wlA")}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlDeletedReopened"), qsl("0:0:260:170"));

        // Saved, it is not held for later saves: one into a fresh file has nothing of it
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("resizeWindow('wlA', 250, 160); assert(deleteMiniConsole('wlA')); assert(saveWindowLayout())")));
        QVERIFY(QFile::remove(layoutFile));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(
                qsl("assert(saveWindowLayout()); openUserWindow('wlA', true, false, 'f'); wlDeletedReopened = table.concat({getWindowGeometry('wlA')}, ':')")));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlDeletedReopened"), qsl("0:0:%1:%2").arg(opened.width()).arg(opened.height()));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(layoutFile));
    }

    // As the GUI, where restoreState() replaces each deleted dock's placeholder with the file's placement or with none
    void test_headlessWindowLayoutLoadDropsDeletedWindowsPlacement()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Deleted-Then-Loaded");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(saveWindowLayout())
resizeWindow("wlA", 260, 170)
assert(deleteMiniConsole("wlA"))
openUserWindow("wlB", false, false, "f")
resizeWindow("wlB", 240, 150)
assert(deleteMiniConsole("wlB"))
assert(loadWindowLayout())
assert(saveWindowLayout())
openUserWindow("wlA", true, false, "f")
openUserWindow("wlB", true, false, "f")
wlDeletedThenLoaded = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getWindowGeometry("wlB")}, ":")
)lua")));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlDeletedThenLoaded"), qsl("0:0:300:200|0:0:%1:%2").arg(opened.width()).arg(opened.height()));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(layoutFile));
    }

    void test_headlessWindowLayoutKeepsDeletedWindowThroughRefusedSave()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Deleted-Save-Refused");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFilePath = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFilePath);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(deleteMiniConsole("wlA"))
)lua")));
        QVERIFY(writeHeadlessLayout(laterVersionLayout()));
        ignoreRefusedLayoutSave();
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("wlRefusedSave = tostring(saveWindowLayout())")));
        QCOMPARE(luaGlobalString(host, "wlRefusedSave"), qsl("false"));
        QVERIFY(QFile::remove(layoutFilePath));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(
                qsl("assert(saveWindowLayout()); openUserWindow('wlA', true, false, 'f'); wlRefusedSave = table.concat({getWindowGeometry('wlA')}, ':')")));
        QCOMPARE(luaGlobalString(host, "wlRefusedSave"), qsl("0:0:300:200"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(layoutFilePath));
    }

    // A refused closing save leaves the windows of the profile, which has gone, held through any load for the next save
    // that succeeds
    void test_headlessWindowLayoutKeepsWindowsOfProfileClosedWithRefusedSave()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Close-Refused");
        const QString otherHostname = qsl("Test-Headless-Host-Window-Layout-Close-Refused-Other");
        QVERIFY2(HostManager::self()->addHost(otherHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* other = HostManager::self()->getHost(otherHostname);
        QVERIFY2(other, "The profile is not in the pool.");
        const QString layoutFilePath = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        auto writeLayoutFile = writeHeadlessLayout;
        auto closeWithRefusedSave = [&](const int width, const int height) {
            QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
            Host* host = HostManager::self()->getHost(hostname);
            QVERIFY2(host, "The profile is not in the pool.");
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', true, false, 'f'); resizeWindow('wlA', %1, %2)").arg(width).arg(height)));
            QVERIFY(writeLayoutFile(laterVersionLayout()));
            ignoreRefusedLayoutSave();
            HostManager::self()->deleteHost(hostname);
        };
        auto reopened = [&]() {
            if (!HostManager::self()->addHost(hostname, QString(), QString(), QString())) {
                return qsl("not added");
            }
            Host* host = HostManager::self()->getHost(hostname);
            host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', true, false, 'f'); wlCloseRefused = table.concat({getWindowGeometry('wlA')}, ':')"));
            const QString geometry = luaGlobalString(host, "wlCloseRefused");
            HostManager::self()->deleteHost(hostname);
            return geometry;
        };

        QFile::remove(layoutFilePath);
        closeWithRefusedSave(300, 200);
        QVERIFY(QFile::remove(layoutFilePath));
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(saveWindowLayout())")));
        QCOMPARE(reopened(), qsl("0:0:300:200"));

        closeWithRefusedSave(250, 160);
        // A readable file, version 1, with no placements
        QVERIFY(writeLayoutFile(QByteArray::fromHex("4d574c48000000010000000000000000")));
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(loadWindowLayout()); assert(saveWindowLayout())")));
        QCOMPARE(reopened(), qsl("0:0:250:160"));
        HostManager::self()->deleteHost(otherHostname);
        QVERIFY(QFile::remove(layoutFilePath));
    }

    // A save writes an open window over where one of its name was deleted, and keeps whether a deleted one showed
    void test_headlessWindowLayoutSavesOpenWindowOverDeletedOne()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Recreated-Saved");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QFile::remove(headlessLayoutPath());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(deleteMiniConsole("wlA"))
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 400, 330)
assert(saveWindowLayout())
resizeWindow("wlA", 500, 400)
assert(loadWindowLayout())
openUserWindow("wlB", false, false, "f")
resizeWindow("wlB", 222, 111)
hideWindow("wlB")
assert(deleteMiniConsole("wlB"))
assert(saveWindowLayout())
openUserWindow("wlB", false, false, "f")
assert(loadWindowLayout())
wlRecreatedSaved = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getWindowGeometry("wlB")}, ":") .. ":" .. tostring(windowVisible("wlB"))
)lua")));
        QCOMPARE(luaGlobalString(host, "wlRecreatedSaved"), qsl("0:0:400:330|0:0:222:111:false"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A profile whose closing save was refused gets its windows back as it reopens, whether the file is then gone or
    // an older one is back, as those placements are newer than any the file can hold
    void test_headlessWindowLayoutLoadsWindowsHeldAsProfileClosed()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Held-Reopened");
        auto run = [&hostname](const QString& chunk, const char* name) {
            Host* host = HostManager::self()->getHost(hostname);
            return host && host->getLuaInterpreter()->compileAndExecuteScript(chunk) ? luaGlobalString(host, name) : qsl("did not run");
        };
        QFile::remove(headlessLayoutPath());

        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        run(qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 300, 200); openMapWidget(50, 60, 333, 222)"), "");
        QVERIFY(writeHeadlessLayout(laterVersionLayout()));
        ignoreRefusedLayoutSave();
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        QCOMPARE(run(qsl("openUserWindow('wlA', true, false, 'f'); openMapWidget(); wlHeld = table.concat({getWindowGeometry('wlA')}, ':') .. '|' .. "
                         "table.concat({getMapWidgetGeometry()}, ':')"),
                     "wlHeld"),
                 qsl("0:0:300:200|50:60:333:222"));
        // Held until a save succeeds, but older than the profile's own windows and its deleted ones
        const QString placed = qsl("wlHeld = table.concat({getWindowGeometry('wlA')}, ':') .. '|' .. table.concat({getMapWidgetGeometry()}, ':')");
        QCOMPARE(run(qsl("resizeWindow('wlA', 222, 111); openMapWidget(70, 80, 111, 99); assert(saveWindowLayout()); resizeWindow('wlA', 400, 330); "
                         "openMapWidget(10, 20, 300, 200); assert(loadWindowLayout()); ")
                             + placed,
                     "wlHeld"),
                 qsl("0:0:222:111|70:80:111:99"));
        run(qsl("resizeWindow('wlA', 260, 170)"), "");
        QVERIFY(writeHeadlessLayout(laterVersionLayout()));
        ignoreRefusedLayoutSave();
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        QCOMPARE(run(qsl("openUserWindow('wlA', true, false, 'f'); resizeWindow('wlA', 222, 111); assert(deleteMiniConsole('wlA')); assert(saveWindowLayout()); "
                         "openUserWindow('wlA', true, false, 'f'); wlHeld = table.concat({getWindowGeometry('wlA')}, ':')"),
                     "wlHeld"),
                 qsl("0:0:222:111"));
        HostManager::self()->deleteHost(hostname);

        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        run(qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 111, 77); assert(saveWindowLayout()); resizeWindow('wlA', 250, 160)"), "");
        QFile layoutFile(headlessLayoutPath());
        QVERIFY(layoutFile.open(QIODevice::ReadOnly));
        const QByteArray older = layoutFile.readAll();
        layoutFile.close();
        QVERIFY(writeHeadlessLayout(laterVersionLayout()));
        ignoreRefusedLayoutSave();
        HostManager::self()->deleteHost(hostname);
        QVERIFY(writeHeadlessLayout(older));
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        QCOMPARE(run(qsl("openUserWindow('wlA', true, false, 'f'); wlHeld = table.concat({getWindowGeometry('wlA')}, ':')"), "wlHeld"), qsl("0:0:250:160"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // Only the pool self() answers with ending tries a last save of what is held, saying what it loses if that fails;
    // cleanupTestCase() checks that a pool outliving it, which cannot save, says what it drops
    void test_headlessWindowLayoutHoldsUntilTheirPoolEnds()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Held-To-Pool-End");
        auto closeWithRefusedSave = [&hostname](const int width, const int height) {
            QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
            QVERIFY(HostManager::self()->getHost(hostname)->getLuaInterpreter()->compileAndExecuteScript(
                    qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', %1, %2)").arg(width).arg(height)));
            QVERIFY(writeHeadlessLayout(laterVersionLayout()));
            ignoreRefusedLayoutSave();
            HostManager::self()->deleteHost(hostname);
        };
        QString opened;
        auto reopened = [&hostname, &opened]() {
            if (!HostManager::self()->addHost(hostname, QString(), QString(), QString())) {
                return qsl("not added");
            }
            Host* host = HostManager::self()->getHost(hostname);
            host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', true, false, 'f'); wlPoolEnd = table.concat({getWindowGeometry('wlA')}, ':')"));
            const QString geometry = luaGlobalString(host, "wlPoolEnd");
            const QSize size = host->mainWindowSize().value_or(QSize());
            opened = qsl("0:0:%1:%2").arg(size.width()).arg(size.height());
            HostManager::self()->deleteHost(hostname);
            return geometry;
        };
        QFile::remove(headlessLayoutPath());

        closeWithRefusedSave(300, 200);
        {
            // A second pool, as in test_exitEventIsRaisedOnceAsTheProfileGoes()
            auto pool = std::make_unique<HostManager>();
        }
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        QCOMPARE(reopened(), qsl("0:0:300:200"));

        closeWithRefusedSave(250, 160);
        QVERIFY(QFile::remove(headlessLayoutPath()));
        // Stands in for self()'s pool ending
        TAppFrontend::instance()->profilesClosed();
        QVERIFY2(QFile::exists(headlessLayoutPath()), "The pool's end saved nothing of what was held.");
        QCOMPARE(reopened(), qsl("0:0:250:160"));

        closeWithRefusedSave(240, 150);
        ignoreRefusedLayoutSave();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("could not be saved, so where these were is lost: QList\\(\"%1/wlA\"\\)$").arg(hostname)));
        TAppFrontend::instance()->profilesClosed();
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        QCOMPARE(reopened(), opened);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // Answers as the same Lua does in the GUI self-test profile, but for the docked geometry, which there is its dock's: a
    // load, and the first openMapWidget() after a reopen, put the map widget back where it was
    void test_headlessWindowLayoutPlacesMapWidget()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Placed");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QFile::remove(headlessLayoutPath());
        const QString geometry = qsl(R"lua(
function wlMapGeometry()
  local x, y, w, h = getMapWidgetGeometry()
  return table.concat({tostring(x), tostring(y), tostring(w), tostring(h)}, ":")
end
)lua");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(geometry + qsl(R"lua(
local answers = {}
local function record(value) answers[#answers + 1] = tostring(value) end
local ok, err = pcall(function()
record(openMapWidget())
local docked = wlMapGeometry()
assert(saveWindowLayout())
openMapWidget(10, 20, 300, 200)
assert(loadWindowLayout())
record(wlMapGeometry() == docked)
openMapWidget(50, 60, 333, 222)
assert(saveWindowLayout())
-- as moveMapWidget() and resizeMapWidget() do, which this profile, with no mudlet-lua, lacks
openMapWidget(120, 130)
openMapWidget(-1, -1, 444, 255)
record(wlMapGeometry())
assert(loadWindowLayout())
record(wlMapGeometry())
closeMapWidget()
assert(saveWindowLayout())
openMapWidget()
record(wlMapGeometry())
assert(loadWindowLayout())
record(wlMapGeometry())
openMapWidget()
assert(saveWindowLayout())
closeMapWidget()
assert(loadWindowLayout())
record(wlMapGeometry())
closeMapWidget()
end)
wlMapPlaced = ok and table.concat(answers, "|") or tostring(err)
)lua")));
        QCOMPARE(luaGlobalString(host, "wlMapPlaced"), qsl("true|true|120:130:444:255|50:60:333:222|50:60:333:222|nil:no floating/dockable type map window found:nil:nil|50:60:333:222"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(geometry + qsl("wlMapPlaced = tostring(openMapWidget()) .. '|' .. wlMapGeometry()")));
        QCOMPARE(luaGlobalString(host, "wlMapPlaced"), qsl("true|50:60:333:222"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As the GUI, where createMapper() deleting the hidden map dock leaves a placeholder a save writes, so the map widget
    // comes back where it was when replaced; with no file, it opens docked at the main window's size
    void test_headlessWindowLayoutKeepsMapWidgetReplacedByEmbeddedMapper()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Replaced");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QFile::remove(headlessLayoutPath());
        const QString geometry = qsl(R"lua(
function wlMapGeometry()
  local x, y, w, h = getMapWidgetGeometry()
  return table.concat({tostring(x), tostring(y), tostring(w), tostring(h)}, ":")
end
)lua");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(geometry + qsl(R"lua(
local ok, err = pcall(function()
  assert(openMapWidget())
  wlMapDocked = wlMapGeometry()
  assert(openMapWidget(50, 60, 333, 222))
  assert(saveWindowLayout())
  assert(openMapWidget(120, 130))
  assert(closeMapWidget())
  assert(createMapper(0, 0, 100, 100))
  assert(saveWindowLayout())
end)
wlMapReplaced = ok and "ok" or tostring(err)
)lua")));
        QCOMPARE(luaGlobalString(host, "wlMapReplaced"), qsl("ok"));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlMapDocked"), qsl("0:0:%1:%2").arg(opened.width()).arg(opened.height()));
        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(geometry + qsl("wlMapReplaced = tostring(openMapWidget()) .. '|' .. wlMapGeometry()")));
        QCOMPARE(luaGlobalString(host, "wlMapReplaced"), qsl("true|120:130:333:222"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A load of an unreadable file still places a closed profile's held windows, which its next close keeps, and, as a
    // failed restoreState() leaves the GUI's placeholders, keeps an open profile's deleted window
    void test_headlessWindowLayoutLoadsHeldWindowsPastUnreadableFile()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Held-Unreadable");
        const QString otherHostname = qsl("Test-Headless-Host-Window-Layout-Held-Unreadable-Other");
        QVERIFY2(HostManager::self()->addHost(otherHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* other = HostManager::self()->getHost(otherHostname);
        QVERIFY2(other, "The profile is not in the pool.");
        auto run = [&hostname](const QString& chunk, const char* name) {
            Host* host = HostManager::self()->getHost(hostname);
            return host && host->getLuaInterpreter()->compileAndExecuteScript(chunk) ? luaGlobalString(host, name) : qsl("did not run");
        };
        const QString reopen = qsl("openUserWindow('wlA', true, false, 'f'); wlHeldUnreadable = table.concat({getWindowGeometry('wlA')}, ':')");
        QFile::remove(headlessLayoutPath());

        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        run(qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 300, 200)"), "");
        QVERIFY(writeHeadlessLayout(laterVersionLayout()));
        ignoreRefusedLayoutSave();
        HostManager::self()->deleteHost(hostname);
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlB', false, false, 'f'); resizeWindow('wlB', 222, 111); assert(deleteMiniConsole('wlB'))")));
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        ignoreLaterVersionLayout();
        QCOMPARE(run(reopen, "wlHeldUnreadable"), qsl("0:0:300:200"));
        ignoreRefusedLayoutSave();
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        QCOMPARE(run(reopen, "wlHeldUnreadable"), qsl("0:0:300:200"));
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlB', true, false, 'f'); wlHeldUnreadable = table.concat({getWindowGeometry('wlB')}, ':')")));
        QCOMPARE(luaGlobalString(other, "wlHeldUnreadable"), qsl("0:0:222:111"));
        HostManager::self()->deleteHost(hostname);
        HostManager::self()->deleteHost(otherHostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A view taking over drops the null view's windows, which no script deleted, so holds no placeholder for them; this
    // calls dropWindows() as setMainConsoleView() does, as a TMainConsole needs a main window this test lacks
    void test_headlessWindowLayoutHoldsNothingOfWindowsAViewTakesOver()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-View-Takes-Over");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY2(!host->hasConsoleView(), "The profile has a console view.");
        QFile::remove(headlessLayoutPath());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 250, 160)
openUserWindow("wlB", false, false, "f")
resizeWindow("wlB", 222, 111)
assert(deleteMiniConsole("wlB"))
)lua")));
        static_cast<TNullConsoleFrontend*>(host->consoleFrontend())->dropWindows();
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        const auto saved = savedUserWindows();
        QCOMPARE(saved.value(hostname + qsl("/wlB")).first, QRect(0, 0, 222, 111));
        QVERIFY2(!saved.contains(hostname + qsl("/wlA")), "A window a view took over was saved as one a script deleted.");
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A closing profile's deleted window and replaced map widget are held with its open ones, so a later load, which
    // drops the placeholders of deleted ones, leaves them for the next save that succeeds
    void test_headlessWindowLayoutHoldsDeletedWindowsOfClosedProfile()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Deleted-Then-Closed");
        const QString otherHostname = qsl("Test-Headless-Host-Window-Layout-Deleted-Then-Closed-Other");
        QVERIFY2(HostManager::self()->addHost(otherHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* other = HostManager::self()->getHost(otherHostname);
        QVERIFY2(other, "The profile is not in the pool.");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local ok, err = pcall(function()
  openUserWindow("wlA", false, false, "f")
  resizeWindow("wlA", 250, 160)
  assert(deleteMiniConsole("wlA"))
  assert(openMapWidget(50, 60, 333, 222))
  assert(closeMapWidget())
  assert(createMapper(0, 0, 100, 100))
end)
wlDeletedClosed = ok and "ok" or tostring(err)
)lua")));
        QCOMPARE(luaGlobalString(host, "wlDeletedClosed"), qsl("ok"));
        QVERIFY(writeHeadlessLayout(laterVersionLayout()));
        ignoreRefusedLayoutSave();
        HostManager::self()->deleteHost(hostname);
        // A readable file, version 1, with no placements
        QVERIFY(writeHeadlessLayout(QByteArray::fromHex("4d574c48000000010000000000000000")));
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(loadWindowLayout()); assert(saveWindowLayout())")));
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", true, false, "f")
openMapWidget()
wlDeletedClosed = table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getMapWidgetGeometry()}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlDeletedClosed"), qsl("0:0:250:160|50:60:333:222"));
        HostManager::self()->deleteHost(hostname);
        HostManager::self()->deleteHost(otherHostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A handler opening the map widget mid-load gets it docked, as the GUI's does, its own load being refused
    void test_headlessWindowLayoutDocksMapWidgetOpenedDuringLoad()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Opened-In-Load");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY(HostManager::self()->getHost(hostname)->getLuaInterpreter()->compileAndExecuteScript(
                qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 300, 200); assert(openMapWidget(50, 60, 333, 222)); assert(saveWindowLayout())")));
        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        QTest::failOnWarning(QRegularExpression(qsl("^TNullAppFrontend::loadWindowLayout")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local armed = false
function onWlResize(_, width, height, name)
  if armed and name == "wlA" then
    armed = false
    openMapWidget()
  end
end
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 400, 330)
armed = true
wlMapInLoad = tostring(loadWindowLayout()) .. "|" .. table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat({getMapWidgetGeometry()}, ":")
)lua")));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlMapInLoad"), qsl("true|0:0:300:200|0:0:%1:%2").arg(opened.width()).arg(opened.height()));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // Placed before the user windows, so a handler their placing runs has the last word on the map widget
    void test_headlessWindowLayoutPlacesMapWidgetBeforeUserWindows()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Before-Windows");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local armed = false
function onWlResize(_, width, height, name)
  if armed and name == "wlA" then
    armed = false
    openMapWidget(70, 80, 111, 99)
  end
end
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(openMapWidget(50, 60, 333, 222))
assert(saveWindowLayout())
resizeWindow("wlA", 400, 330)
openMapWidget(10, 20, 300, 200)
armed = true
wlMapFirst = tostring(loadWindowLayout()) .. "|" .. table.concat({getMapWidgetGeometry()}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlMapFirst"), qsl("true|70:80:111:99"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A reset deletes the user windows as a script would, so each keeps its placeholder, unlike a view taking over
    void test_headlessWindowLayoutHoldsWindowsOfAReset()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Reset");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 250, 160)")));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY2(host->resetProfile_phase1(), "The profile reset did not start.");
        QTRY_VERIFY(!host->profileResetInProgress());
        QVERIFY2(!host->windowRegistry().subConsoleNames().contains(qsl("wlA")), "The reset left the user window.");
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        QCOMPARE(savedUserWindows().value(hostname + qsl("/wlA")).first, QRect(0, 0, 250, 160));
        QTRY_VERIFY(!host->currentlySavingProfile());
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A profile its exit handler deleted, so closed, and reopened under its name is not closed again, which would take
    // the new one's deleted window as its own and write its own over it
    void test_headlessWindowLayoutLeavesProfileReopenedUnderItsExitHandler()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Exit-Reopened");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "reopenWithDeletedWindow", [](lua_State* L) -> int {
            const QString name = QString::fromUtf8(lua_tostring(L, 1));
            HostManager::self()->deleteHost(name);
            Host* pReopened = HostManager::self()->addHost(name, QString(), QString(), QString()) ? HostManager::self()->getHost(name) : nullptr;
            lua_pushboolean(L,
                            pReopened
                                    && pReopened->getLuaInterpreter()->compileAndExecuteScript(
                                            qsl("openUserWindow('wlA', false, false, 'f'); resizeWindow('wlA', 300, 200); assert(deleteMiniConsole('wlA'))")));
            return 1;
        });
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onWlExit"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 250, 160)
function onWlExit() assert(reopenWithDeletedWindow(getProfileName())) end
)lua")));
        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->getHost(hostname), "Closing the profile also removed the one reopened under its name.");
        QVERIFY(TAppFrontend::instance()->saveWindowLayoutForScript());
        QCOMPARE(savedUserWindows().value(hostname + qsl("/wlA")).first, QRect(0, 0, 300, 200));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // A load shows a map widget hidden since the save, as restoreState() shows its dock, so its 2D map paints again
    void test_headlessWindowLayoutShowsMapWidgetHiddenSinceSave()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Shown");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        // The null view paints on short chains of zero timers, so passes rather than time deliver them
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State*) -> int {
            for (int pass = 0; pass < 10; ++pass) {
                QCoreApplication::processEvents();
            }
            return 0;
        });
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local ok, err = pcall(function()
wlShownChanges = {}
function wlShownAreaChanged(_, shown) wlShownChanges[#wlShownChanges + 1] = getRoomAreaName(tonumber(shown)) end
registerAnonymousEventHandler("sysMapAreaChanged", "wlShownAreaChanged")
local function room(area)
  local id = createRoomID()
  addRoom(id)
  setRoomArea(id, addAreaName(area))
  setRoomCoordinates(id, 0, 0, 0)
  return id
end
local near, far, farther = room("WlNear"), room("WlFar"), room("WlFarther")
assert(openMapWidget())
centerview(near)
pumpEvents()
assert(saveWindowLayout())
assert(closeMapWidget())
centerview(far)
pumpEvents()
local hidden = table.concat(wlShownChanges, ",")
wlShownChanges = {}
assert(loadWindowLayout())
centerview(farther)
pumpEvents()
wlMapShown = hidden .. "|" .. table.concat(wlShownChanges, ",")
end)
if not ok then wlMapShown = "raised:" .. tostring(err) end
)lua")));
        QCOMPARE(luaGlobalString(host, "wlMapShown"), qsl("WlNear|WlFarther"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As the GUI, where restoreState() holds a dock it found no window for and saveState() writes it back
    void test_headlessWindowLayoutKeepsUnopenedWindowsPlacement()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Unopened");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
openUserWindow("wlB", false, false, "f")
resizeWindow("wlB", 300, 200)
assert(saveWindowLayout())
)lua")));
        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create the profile again.");
        host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", true, false, "f")
assert(saveWindowLayout())
openUserWindow("wlB", true, false, "f")
wlUnopenedReopened = table.concat({getWindowGeometry("wlB")}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlUnopenedReopened"), qsl("0:0:300:200"));
        QVERIFY(QFile::remove(layoutFile));
    }

    void test_headlessWindowLayoutKeepsFilePlacementOfWindowDeletedMidLoad_data()
    {
        QTest::addColumn<bool>("savedShown");
        QTest::newRow("saved shown") << true;
        QTest::newRow("saved hidden") << false;
    }

    // A window a resize handler deletes before the load reaches it keeps the file's placement, as the placeholder
    // restoreState() gives it does in the GUI, not where it was before the load
    void test_headlessWindowLayoutKeepsFilePlacementOfWindowDeletedMidLoad()
    {
        QFETCH(const bool, savedShown);
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Deleted-Mid-Load");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlDeleteOther"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("wlSavedShown = %1").arg(savedShown ? qsl("true") : qsl("false"))));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlDeletedMidLoad = "not run"
local armed, deleted = false, nil
function onWlDeleteOther(_, _, _, name)
  if armed and not deleted and (name == "wlA" or name == "wlB") then
    deleted = name == "wlA" and "wlB" or "wlA"
    assert(deleteMiniConsole(deleted))
  end
end
local ok, err = pcall(function()
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  if not wlSavedShown then
    assert(hideWindow("wlB"))
  end
  assert(saveWindowLayout())
  assert(showWindow("wlB"))
  resizeWindow("wlA", 400, 330)
  resizeWindow("wlB", 410, 340)
  armed = true
  assert(loadWindowLayout())
  armed = false
  assert(saveWindowLayout())
  wlDeletedName = deleted
  openUserWindow(deleted, true, false, "f")
  wlDeletedMidLoad = deleted .. "|" .. table.concat({getWindowGeometry(deleted)}, ":")
end)
if not ok then wlDeletedMidLoad = tostring(err) end
)lua")));
        const QString answer = luaGlobalString(host, "wlDeletedMidLoad");
        const QString deleted = luaGlobalString(host, "wlDeletedName");
        const bool savedAsShown = savedUserWindows().value(qsl("%1/%2").arg(hostname, deleted)).second;
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QVERIFY2(answer == qsl("wlB|0:0:320:220") || answer == qsl("wlA|0:0:300:200"), qPrintable(answer));
        // wlA always shows: only wlB is saved hidden
        QCOMPARE(savedAsShown, savedShown || deleted == qsl("wlA"));
    }

    void test_headlessWindowLayoutLeavesHiddenWindowItsResizeHandlerHid_data()
    {
        QTest::addColumn<QString>("handler");
        QTest::addColumn<QString>("expected");
        // Each answer: wlA as its handler finds it, both after the handler, and both after the load
        QTest::newRow("hides its own") << qsl("assert(hideWindow('wlA'))") << qsl("false/false,true|false|true");
        // restoreState() reaches wlB after wlA's handler hid it, so shows it as the file says
        QTest::newRow("hides one yet to be placed") << qsl("assert(hideWindow('wlB'))") << qsl("false/false,false|true|true");
        QTest::newRow("replaces its own with a hidden one") << qsl("assert(deleteMiniConsole('wlA')); openUserWindow('wlA', false, false, 'f'); assert(hideWindow('wlA'))")
                                                            << qsl("false/false,true|false|true");
    }

    // As in the GUI, where restoreState() hides a dock before placing it, so its resize handler finds it hidden, and does not
    // show again one the handler hid or replaced
    void test_headlessWindowLayoutLeavesHiddenWindowItsResizeHandlerHid()
    {
        QFETCH(const QString, handler);
        QFETCH(const QString, expected);
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Hidden-Mid-Load");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlHideMidLoad"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlHiddenMidLoad = "not run"
local armed, saw = false, "handler not run"
function onWlHideMidLoad(_, _, _, name)
  if armed and name == "wlA" then
    armed = false
    local found = tostring(windowVisible("wlA"))
    local ok, err = pcall(function()
      %1
    end)
    saw = ok and (found .. "/" .. tostring(windowVisible("wlA")) .. "," .. tostring(windowVisible("wlB"))) or ("handler raised: " .. tostring(err))
  end
end
local ok, err = pcall(function()
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  assert(saveWindowLayout())
  resizeWindow("wlA", 400, 330)
  resizeWindow("wlB", 410, 340)
  armed = true
  assert(loadWindowLayout())
  wlHiddenMidLoad = saw .. "|" .. tostring(windowVisible("wlA")) .. "|" .. tostring(windowVisible("wlB"))
end)
if not ok then wlHiddenMidLoad = tostring(err) end
)lua")
                                                                           .arg(handler)));
        const QString answer = luaGlobalString(host, "wlHiddenMidLoad");
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(headlessLayoutPath()));
        QCOMPARE(answer, expected);
    }

    void test_headlessWindowLayoutTakesWindowBeingShownAsShown_data()
    {
        QTest::addColumn<QString>("setup");
        QTest::addColumn<QString>("handler");
        QTest::addColumn<QString>("after");
        QTest::addColumn<QString>("expected");
        // Each answer: wlA's resize events from its show on, and whether it shows at the end
        QTest::newRow("save") << QString() << qsl("assert(saveWindowLayout())") << qsl("assert(hideWindow('wlA')); assert(loadWindowLayout())") << qsl("1|true");
        QTest::newRow("load") << qsl("assert(hideWindow('wlA')); assert(saveWindowLayout())") << qsl("assert(loadWindowLayout())") << QString() << qsl("1|false");
        // As restoreState() hides it, places it and shows it again, which sends the resize once more
        QTest::newRow("load shown") << qsl("assert(saveWindowLayout())") << qsl("assert(loadWindowLayout())") << QString() << qsl("2|true");
    }

    // A save or load in the handler of the resize a show sends takes the window as shown, as the GUI's !isHidden() does,
    // though it can answer hidden to scripts there until a load succeeds
    void test_headlessWindowLayoutTakesWindowBeingShownAsShown()
    {
        QFETCH(const QString, setup);
        QFETCH(const QString, handler);
        QFETCH(const QString, after);
        QFETCH(const QString, expected);
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Being-Shown");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlBeingShown"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlBeingShown = "not run"
local armed, events = false, 0
function onWlBeingShown(_, _, _, name)
  if name == "wlA" then
    events = events + 1
    if armed then
      armed = false
      %2
    end
  end
end
local ok, err = pcall(function()
  openUserWindow("wlA", false, false, "f")
  resizeWindow("wlA", 300, 200)
  %1
  hideWindow("wlA")
  resizeWindow("wlA", 260, 170)
  events = 0
  armed = true
  assert(showWindow("wlA"))
  %3
  wlBeingShown = events .. "|" .. tostring(windowVisible("wlA"))
end)
if not ok then wlBeingShown = tostring(err) end
)lua")
                                                                           .arg(setup, handler, after)));
        const QString answer = luaGlobalString(host, "wlBeingShown");
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, expected);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As restoreFloatingDockGeometries() gives a floating dock its saved geometry again once restoreState() is done
    void test_headlessWindowLayoutRestoresGeometryItsResizeHandlerChanged()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Resized-Mid-Load");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResizeMidLoad"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlResizedMidLoad = "not run"
local armed, events = false, {}
function onWlResizeMidLoad(_, width, height, name)
  if name == "wlA" then
    events[#events + 1] = width .. "x" .. height .. ":" .. tostring(windowVisible(name))
    if armed then
      armed = false
      moveWindow("wlA", 77, 88)
      resizeWindow("wlA", 222, 133)
    end
  end
end
local ok, err = pcall(function()
  openUserWindow("wlA", false, false, "f")
  moveWindow("wlA", 31, 41)
  resizeWindow("wlA", 300, 200)
  assert(saveWindowLayout())
  resizeWindow("wlA", 260, 170)
  events = {}
  armed = true
  assert(loadWindowLayout())
  wlResizedMidLoad = table.concat(events, ";") .. "|" .. table.concat({getUserWindowSize("wlA")}, "x")
end)
if not ok then wlResizedMidLoad = tostring(err) end
)lua")));
        const QString answer = luaGlobalString(host, "wlResizedMidLoad");
        const QPoint position = host->windowRegistry().subConsoleGeometry(qsl("wlA")).value_or(QRect()).topLeft();
        // Before comparing, so a failure leaves later tests its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, qsl("300x200:false;300x200:true|300x200"));
        QCOMPARE(position, QPoint(31, 41));
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As restoreFloatingDockGeometries() places a window a handler opened during restoreState() once, so a size its own
    // handler then gives it stays
    void test_headlessWindowLayoutPlacesWindowAHandlerOpenedOnce()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Handler-Opened-Once");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlOpenedMidLoad"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlOpenedMidLoad = "not run"
local armed, events = false, nil
function onWlOpenedMidLoad(_, width, height, name)
  if name == "wlA" and armed then
    armed = false
    openUserWindow("wlB", false, false, "f")
    events = {}
  elseif name == "wlB" and events then
    events[#events + 1] = width .. "x" .. height
    if #events == 1 then
      resizeWindow("wlB", 222, 133)
    end
  end
end
local ok, err = pcall(function()
  openUserWindow("wlA", false, false, "f")
  resizeWindow("wlA", 300, 200)
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlB", 280, 180)
  assert(saveWindowLayout())
  assert(deleteMiniConsole("wlB"))
  resizeWindow("wlA", 260, 170)
  armed = true
  assert(loadWindowLayout())
  wlOpenedMidLoad = table.concat(events, ";") .. "|" .. table.concat({getUserWindowSize("wlB")}, "x")
end)
if not ok then wlOpenedMidLoad = tostring(err) end
)lua")));
        const QString answer = luaGlobalString(host, "wlOpenedMidLoad");
        // Before comparing, so a failure leaves later tests its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, qsl("280x180;222x133|222x133"));
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As TMainConsole::reportDockGeometry() reports each dock's name, so also a window of that name the showing one
    // holds, which a hidden dock's delete leaves to its deferred deletes
    void test_headlessWindowLayoutLoadReportsUserWindowNames()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Reports-Names");
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlReportsNames"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlReportsNames = "not run"
local armed, found = false, "not run"
function onWlReportsNames(_, _, _, name)
  if name == "wlShowing" and armed then
    armed = false
    found = tostring(loadWindowLayout()) .. "," .. tostring(deleteMiniConsole("wlHidden")) .. "," .. tostring(windowVisible("wlHidden"))
  end
end
local ok, err = pcall(function()
  openUserWindow("wlHidden", false, false, "f")
  hideWindow("wlHidden")
  assert(saveWindowLayout())
  openUserWindow("wlShowing", false, false, "f")
  createTextEdit("wlShowing", "wlHidden", 0, 0, 50, 50)
  hideWindow("wlShowing")
  resizeWindow("wlShowing", 260, 170)
  armed = true
  showWindow("wlShowing")
  wlReportsNames = found
end)
if not ok then wlReportsNames = tostring(err) end
)lua")));
        const QString answer = luaGlobalString(host, "wlReportsNames");
        // Before comparing, so a failure leaves later tests its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, qsl("true,true,true"));
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // HostManager::addHost() takes a '/' or '\' that dlgConnectionProfiles refuses, so a profile name holding one must
    // not share a window's key with another profile
    void test_headlessWindowLayoutKeepsProfilesWithSlashesApart_data()
    {
        QTest::addColumn<QString>("outer");
        QTest::addColumn<QString>("outerWindow");
        QTest::addColumn<QString>("inner");
        QTest::addColumn<QString>("innerWindow");
        // Without escaping, the first pair would share a key; escaping only '/', the second would
        QTest::newRow("slash") << qsl("Test-Headless-Host-Layout-Key") << qsl("x/w") << qsl("Test-Headless-Host-Layout-Key/x") << qsl("w");
        QTest::newRow("backslash") << qsl("Test-Headless-Host-Layout-Key\\") << qsl("/w") << qsl("Test-Headless-Host-Layout-Key/") << qsl("w");
    }

    void test_headlessWindowLayoutKeepsProfilesWithSlashesApart()
    {
        QFETCH(const QString, outer);
        QFETCH(const QString, outerWindow);
        QFETCH(const QString, inner);
        QFETCH(const QString, innerWindow);
        QFile::remove(headlessLayoutPath());
        QVERIFY2(HostManager::self()->addHost(outer, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(HostManager::self()->addHost(inner, QString(), QString(), QString()), "Could not create a profile whose name holds a '/'.");
        Host* outerHost = HostManager::self()->getHost(outer);
        Host* innerHost = HostManager::self()->getHost(inner);
        QVERIFY2(outerHost && innerHost, "The profiles are not in the pool.");
        const auto run = [](Host* host, const QString& lua) {
            return host->getLuaInterpreter()->compileAndExecuteScript(qsl("local ok, err = pcall(function() %1 end) keyResult = ok and keyResult or tostring(err)").arg(lua));
        };
        QVERIFY(run(outerHost, qsl("openUserWindow('%1', false, false, 'f'); resizeWindow('%1', 300, 200)").arg(outerWindow)));
        QVERIFY(run(innerHost, qsl("openUserWindow('%1', false, false, 'f'); resizeWindow('%1', 280, 180)").arg(innerWindow)));
        QVERIFY(run(outerHost, qsl("assert(saveWindowLayout()); resizeWindow('%1', 260, 170)").arg(outerWindow)));
        QVERIFY(run(innerHost, qsl("resizeWindow('%1', 250, 150)").arg(innerWindow)));
        QVERIFY(run(outerHost, qsl("assert(loadWindowLayout()); keyResult = table.concat({getUserWindowSize('%1')}, 'x')").arg(outerWindow)));
        QVERIFY(run(innerHost, qsl("keyResult = table.concat({getUserWindowSize('%1')}, 'x')").arg(innerWindow)));
        const QString answer = luaGlobalString(outerHost, "keyResult") + qsl("|") + luaGlobalString(innerHost, "keyResult");
        // Before comparing, so a failure leaves later tests their profile names
        HostManager::self()->deleteHost(inner);
        HostManager::self()->deleteHost(outer);
        QCOMPARE(answer, qsl("300x200|280x180"));
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // As a new TDockWidget's hasLayoutAlready, nothing of a deleted window stops its next open with the layout loading it
    void test_headlessWindowLayoutLoadsForRecreatedWindow()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Recreated");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", true, false, "f")
resizeWindow("wlA", 300, 200)
assert(saveWindowLayout())
resizeWindow("wlA", 400, 330)
assert(deleteMiniConsole("wlA"))
openUserWindow("wlA", true, false, "f")
wlRecreated = table.concat({getWindowGeometry("wlA")}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlRecreated"), qsl("0:0:300:200"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the frame and positions it reports
    void test_headlessWindowLayoutLoadedByFirstMapWidget()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Widget");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local events = {}
function onWlResize(_, width, height, name)
  events[#events + 1] = table.concat({width, height, name}, ":")
end
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(saveWindowLayout())
resizeWindow("wlA", 400, 330)
local answers = {}
for _ = 1, 2 do
  events = {}
  local opened = openMapWidget()
  answers[#answers + 1] = tostring(opened) .. "|" .. table.concat({getWindowGeometry("wlA")}, ":") .. "|" .. table.concat(events, ";")
  resizeWindow("wlA", 400, 330)
end
wlMapWidget = table.concat(answers, "\n")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlMapWidget"), qsl("true|0:0:300:200|300:200:wlA\ntrue|0:0:400:330|"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // As the GUI, which docks the first map widget before the load and shows it after, so a handler's placement stands
    void test_headlessWindowLayoutLeavesMapWidgetWhereLoadHandlerPutIt()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Map-Widget-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlMapResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local armed = false
function onWlMapResize(_, _, _, name)
  if armed and name == "wlA" then
    armed = false
    openMapWidget(10, 20, 400, 300)
  end
end
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(saveWindowLayout())
resizeWindow("wlA", 400, 330)
armed = true
wlMapHandler = tostring(openMapWidget()) .. "|" .. tostring(armed) .. "|" .. table.concat({getMapWidgetGeometry()}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlMapHandler"), qsl("true|false|10:20:400:300"));
        HostManager::self()->deleteHost(hostname);
        QVERIFY(QFile::remove(layoutFile));
    }

    // As the GUI: resized before it is shown, so it reports nothing, being back at the size it last reported
    void test_headlessWindowLayoutShowsHiddenWindowAtSavedSize()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Hidden-Resized");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local events = {}
function onWlResize(_, width, height, name)
  events[#events + 1] = table.concat({width, height, name}, ":")
end
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
assert(saveWindowLayout())
hideWindow("wlA")
resizeWindow("wlA", 400, 330)
events = {}
local loaded = loadWindowLayout()
wlHiddenResized = tostring(loaded) .. "|" .. table.concat({getWindowGeometry("wlA")}, ":") .. ":" .. tostring(windowVisible("wlA")) .. "|" .. table.concat(events, ";")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlHiddenResized"), qsl("true|0:0:300:200:true|"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // As the GUI, which saves and restores the geometry of docks only, so a deleted user window's is kept while a
    // miniconsole has its name
    void test_headlessWindowLayoutLeavesMiniConsoleOfUserWindowsName()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Kind");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 300, 200)
hideWindow("wlA")
assert(saveWindowLayout())
assert(deleteMiniConsole("wlA"))
createMiniConsole("wlA", 10, 20, 150, 100)
local loaded = loadWindowLayout()
wlKind = tostring(loaded) .. "|" .. tostring(windowType("wlA")) .. "|" .. table.concat({getWindowGeometry("wlA")}, ":") .. ":" .. tostring(windowVisible("wlA"))
assert(saveWindowLayout())
assert(deleteMiniConsole("wlA"))
openUserWindow("wlA", true, false, "f")
wlKind = wlKind .. "|" .. table.concat({getWindowGeometry("wlA")}, ":")
)lua")));
        QCOMPARE(luaGlobalString(host, "wlKind"), qsl("true|miniconsole|10:20:150:100:true|0:0:300:200"));
        QVERIFY(QFile::remove(layoutFile));
    }

    // As the GUI's restoreState() and then restoreFloatingDockGeometries(), a load places in two passes, so a
    // handler that reopens its window each time the load resizes it cannot keep it going
    void test_headlessWindowLayoutStopsPlacingReopenedWindows()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Reopened-Always");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QTest::failOnWarning(QRegularExpression(qsl("^TNullAppFrontend::loadWindowLayout")));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlAlwaysResult = "not run"
local ok, err = pcall(function()
  local armed, reopened = false, 0
  -- only at the load's resize, not the reopened window's own, and bounded so a regression fails rather than hangs
  function onWlResize(_, width, _, name)
    if armed and name == "wlA" and width == 300 and reopened < 50 then
      reopened = reopened + 1
      assert(deleteMiniConsole("wlA"))
      openUserWindow("wlA", false, false, "f")
    end
  end
  openUserWindow("wlA", false, false, "f")
  resizeWindow("wlA", 300, 200)
  assert(saveWindowLayout())
  resizeWindow("wlA", 400, 330)
  armed = true
  assert(loadWindowLayout())
  armed = false
  wlAlwaysAnswers = reopened .. "|" .. table.concat({getWindowGeometry("wlA")}, ":")
end)
wlAlwaysResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlAlwaysResult"), qsl("ok"));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlAlwaysAnswers"), qsl("2|0:0:%1:%2").arg(opened.width()).arg(opened.height()));
        QVERIFY(QFile::remove(layoutFile));
    }

    // A chain of windows each opened at the previous one's resize ends with the second pass, as in the GUI, leaving the
    // one opened in it where it opened
    void test_headlessWindowLayoutLeavesWindowsOpenedInSecondPass()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Chain");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        QTest::failOnWarning(QRegularExpression(qsl("^TNullAppFrontend::loadWindowLayout")));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlChainResult = "not run"
local ok, err = pcall(function()
  local armed = false
  function onWlResize(_, width, _, name)
    if not armed then return end
    if name == "pA" then
      openUserWindow("pB", true, false, "f")
    elseif name == "pB" and width == 320 then
      openUserWindow("pC", true, false, "f")
    end
  end
  for _, window in ipairs({{"pA", 300, 200}, {"pB", 320, 220}, {"pC", 330, 230}}) do
    openUserWindow(window[1], false, false, "f")
    resizeWindow(window[1], window[2], window[3])
  end
  assert(saveWindowLayout())
  assert(deleteMiniConsole("pB"))
  assert(deleteMiniConsole("pC"))
  resizeWindow("pA", 400, 330)
  armed = true
  local loaded = loadWindowLayout()
  armed = false
  local sizes = {}
  for _, window in ipairs({"pA", "pB", "pC"}) do
    sizes[#sizes + 1] = table.concat({getWindowGeometry(window)}, ":")
  end
  wlChainAnswers = tostring(loaded) .. "|" .. table.concat(sizes, "|")
end)
wlChainResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlChainResult"), qsl("ok"));
        const QSize opened = host->mainWindowSize().value_or(QSize());
        QCOMPARE(luaGlobalString(host, "wlChainAnswers"), qsl("true|0:0:300:200|0:0:320:220|0:0:%1:%2").arg(opened.width()).arg(opened.height()));
        QVERIFY(QFile::remove(layoutFile));
    }

    // A window freed mid-load can leave its address to one a handler opens, which must not pass for the one the load
    // found: as a dock made during the GUI's restoreState(), the new one takes its saved geometry, not its hidden state
    void test_headlessWindowLayoutTellsReplacedWindowFromFreedOne()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Layout-Replaced");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "replaceWlBAtItsAddress", [](lua_State* L) -> int {
            Host& profile = getHostFromLua(L);
            const QString name = qsl("wlB");
            const void* original = &profile.windowRegistry().subConsoleModel(name)->mNotifier;
            const char* outcome = "never reused";
            for (int attempt = 0; attempt < 20; ++attempt) {
                const QPointer<const QObject> freed = &profile.windowRegistry().subConsoleModel(name)->mNotifier;
                if (!profile.deleteMiniConsole(name).first) {
                    outcome = "not deleted";
                    break;
                }
                for (int pass = 0; freed && pass < 100; ++pass) {
                    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                }
                if (freed) {
                    outcome = "not freed";
                    break;
                }
                if (!profile.openWindow(name, false, false, qsl("f")).first) {
                    outcome = "not reopened";
                    break;
                }
                if (&profile.windowRegistry().subConsoleModel(name)->mNotifier == original) {
                    outcome = "reused";
                    break;
                }
            }
            lua_pushstring(L, outcome);
            return 1;
        });
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onWlResize"));
        const QString layoutFile = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutFile);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
wlReplacedResult = "not run"
local ok, err = pcall(function()
  local armed = false
  function onWlResize(_, _, _, name)
    if armed and name == "wlA" then
      armed = false
      wlReplaced = replaceWlBAtItsAddress()
    end
  end
  openUserWindow("wlA", false, false, "f")
  openUserWindow("wlB", false, false, "f")
  resizeWindow("wlA", 300, 200)
  resizeWindow("wlB", 320, 220)
  hideWindow("wlB")
  assert(saveWindowLayout())
  resizeWindow("wlA", 400, 330)
  armed = true
  assert(loadWindowLayout())
  assert(not armed, "the handler did not run")
  wlReplacedAnswers = table.concat({getWindowGeometry("wlB")}, ":") .. ":" .. tostring(windowVisible("wlB"))
end)
wlReplacedResult = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "wlReplacedResult"), qsl("ok"));
        QVERIFY(QFile::remove(layoutFile));
        if (luaGlobalString(host, "wlReplaced") == qsl("never reused")) {
            QSKIP("No reopened window took the freed one's address, so there was nothing to tell apart.");
        }
        QCOMPARE(luaGlobalString(host, "wlReplaced"), qsl("reused"));
        QCOMPARE(luaGlobalString(host, "wlReplacedAnswers"), qsl("0:0:320:220:true"));
    }

    void test_headlessUserWindowNamesInItsShowHandler_data()
    {
        QTest::addColumn<QString>("before");
        QTest::addColumn<QString>("during");
        QTest::addColumn<QString>("expected");
        // As in the GUI, where a delete reports the name it frees, and a change to a scroll box or user window reports
        // each name it holds, so also a window of that name elsewhere
        const QString window = qsl("return windowVisible('uwn')");
        const QString mini = qsl("return windowVisible('uwnMini')");
        const QString boxed = qsl("createScrollBox('uwnBox', 0, 300, 80, 80); createTextEdit('uwnBox', 'uwnMini', 0, 0, 50, 50)");
        QTest::newRow("same-named command line deleted") << qsl("enableCommandLine('uwn')") << qsl("deleteCommandLine('uwn'); %1").arg(window) << qsl("true|true|true");
        QTest::newRow("same-named scroll box deleted") << qsl("createScrollBox('uwn', 'uwn', 0, 0, 50, 50)") << qsl("deleteScrollBox('uwn'); %1").arg(window) << qsl("true|true|true");
        QTest::newRow("same-named text edit deleted") << qsl("createTextEdit('uwn', 'uwn', 0, 0, 50, 50)") << qsl("deleteTextEdit('uwn'); %1").arg(window) << qsl("true|true|true");
        QTest::newRow("held window's namesake deleted") << qsl("createCommandLine('uwnMini', 0, 300, 50, 20)") << qsl("deleteCommandLine('uwnMini'); %1").arg(mini) << qsl("true|true|true");
        QTest::newRow("scroll box holding a namesake hidden") << boxed << qsl("hideWindow('uwnBox'); %1").arg(mini) << qsl("true|true|true");
        QTest::newRow("scroll box holding a namesake shown") << qsl("%1; hideWindow('uwnBox')").arg(boxed) << qsl("showWindow('uwnBox'); %1").arg(mini) << qsl("true|true|true");
        QTest::newRow("scroll box holding a namesake moved") << boxed << qsl("setWindow('uwnOther', 'uwnBox', 0, 0, true); %1").arg(mini) << qsl("true|true|true");
        QTest::newRow("window holding a namesake hidden") << qsl("createCommandLine('uwnOther', 'uwnMini', 0, 0, 50, 20)") << qsl("hideWindow('uwnOther'); %1").arg(mini) << qsl("true|true|true");
        QTest::newRow("window holding a namesake shown") << qsl("createCommandLine('uwnOther', 'uwnMini', 0, 0, 50, 20); hideWindow('uwnOther')") << qsl("showWindow('uwnOther'); %1").arg(mini)
                                                         << qsl("true|true|true");
        QTest::newRow("window holding a namesake deleted") << qsl("createTextEdit('uwnOther', 'uwnMini', 0, 0, 50, 50)") << qsl("deleteMiniConsole('uwnOther'); %1").arg(mini) << qsl("true|true|true");
        // A hidden dock's removal hides nothing, so what it held reports only as its deferred deletes run
        QTest::newRow("hidden window holding a namesake deleted") << qsl("createTextEdit('uwnOther', 'uwnMini', 0, 0, 50, 50); hideWindow('uwnOther')")
                                                                  << qsl("deleteMiniConsole('uwnOther'); %1").arg(mini) << qsl("false|true|true");
        QTest::newRow("hidden window holding a namesake's command line deleted")
                << qsl("createTextEdit('uwn', 'uwnHeld', 0, 0, 50, 50); createMiniConsole('uwnOther', 'uwnHeld', 0, 0, 50, 50); enableCommandLine('uwnHeld'); hideWindow('uwnOther')")
                << qsl("deleteMiniConsole('uwnOther'); return windowVisible('uwnHeld')") << qsl("false|true|true");
        QTest::newRow("window of a held namesake deleted") << qsl("createTextEdit('uwn', 'uwnOther', 0, 0, 50, 50)") << qsl("deleteMiniConsole('uwnOther'); return windowVisible('uwnOther')")
                                                           << qsl("true|true|true");
        QTest::newRow("hidden window of a held namesake deleted") << qsl("createTextEdit('uwn', 'uwnOther', 0, 0, 50, 50); hideWindow('uwnOther')")
                                                                  << qsl("deleteMiniConsole('uwnOther'); return windowVisible('uwnOther')") << qsl("false|true|true");
        // A delete run inside the pump shows that deferred report
        QTest::newRow("hidden window holding a namesake deleted, then pumped")
                << qsl("createTextEdit('uwnOther', 'uwnMini', 0, 0, 50, 50); hideWindow('uwnOther')") << qsl("tempTimer(0, function() deleteMiniConsole('uwnOther') end); pumpEvents(); %1").arg(mini)
                << qsl("true|true|true");
        QTest::newRow("scroll box holding no namesake hidden") << qsl("createScrollBox('uwnBox', 0, 300, 80, 80); createTextEdit('uwnBox', 'uwnElse', 0, 0, 50, 50)")
                                                               << qsl("hideWindow('uwnBox'); %1").arg(mini) << qsl("false|true|true");
    }

    // As the GUI self-test profile answers the same
    void test_headlessUserWindowNamesInItsShowHandler()
    {
        QFETCH(const QString, before);
        QFETCH(const QString, during);
        QFETCH(const QString, expected);
        const QString hostname = qsl("Test-Headless-Host-User-Window-Names-Show-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onUwnShowHandler"));
        // With no view, pumpEvents() takes Mudlet for shutting down and pumps nothing
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State*) -> int {
            for (int pass = 0; pass < 10; ++pass) {
                QCoreApplication::processEvents();
            }
            return 0;
        });
        QFile::remove(headlessLayoutPath());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
uwnShowHandler = "not run"
local during, found = nil, "not run"
function onUwnShowHandler(_, _, _, name)
  if name == "uwn" and during then
    local act = during
    during = nil
    found = tostring(act())
  end
end
local ok, err = pcall(function()
  openUserWindow("uwn", false, false, "f")
  openUserWindow("uwnOther", false, false, "f")
  createMiniConsole("uwn", "uwnMini", 0, 0, 50, 50)
  %1
  hideWindow("uwn")
  resizeWindow("uwn", 260, 170)
  during = function() %2 end
  local shown = showWindow("uwn")
  uwnShowHandler = found .. "|" .. tostring(shown) .. "|" .. tostring(windowVisible("uwn"))
end)
if not ok then uwnShowHandler = tostring(err) end
)lua")
                                                                           .arg(before, during)));
        const QString answer = luaGlobalString(host, "uwnShowHandler");
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, expected);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    void test_headlessUserWindowCommandLineInItsShowHandler_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<QString>("during");
        QTest::addColumn<QString>("expected");
        // As in the GUI, where a change to the window's own command line reports the window's name, so its dock
        const QString shown = qsl("enableCommandLine('uwl')");
        const QString hidden = qsl("enableCommandLine('uwl'); disableCommandLine('uwl')");
        const QString answer = qsl("return tostring(windowVisible('uwl')) .. ',' .. tostring(windowVisible('uwlOld'))");
        QTest::newRow("first command line") << QString() << qsl("enableCommandLine('uwl'); %1").arg(answer) << qsl("true,false|true|true");
        QTest::newRow("hidden command line shown") << hidden << qsl("enableCommandLine('uwl'); %1").arg(answer) << qsl("true,false|true|true");
        QTest::newRow("shown command line hidden") << shown << qsl("disableCommandLine('uwl'); %1").arg(answer) << qsl("true,false|true|true");
        QTest::newRow("first command line, then a scroll box shown") << QString() << qsl("enableCommandLine('uwl'); showWindow('uwlBox'); %1").arg(answer) << qsl("true,false|true|true");
        // As QWidget::setVisible() to what the command line already is, which reports nothing
        QTest::newRow("shown command line shown again") << shown << qsl("enableCommandLine('uwl'); %1").arg(answer) << qsl("false,false|true|true");
        QTest::newRow("hidden command line hidden again") << hidden << qsl("disableCommandLine('uwl'); %1").arg(answer) << qsl("false,false|true|true");
        QTest::newRow("no command line hidden") << QString() << qsl("disableCommandLine('uwl'); %1").arg(answer) << qsl("false,false|true|true");
        // As the GUI's reportDockGeometry() once a load succeeds, here from a layout without the window
        QTest::newRow("layout loaded, then the shown command line shown again") << shown << qsl("assert(loadWindowLayout()); enableCommandLine('uwl'); %1").arg(answer) << qsl("true,false|true|true");
        QTest::newRow("layout loaded, then the hidden command line shown") << hidden << qsl("assert(loadWindowLayout()); enableCommandLine('uwl'); %1").arg(answer) << qsl("true,false|true|true");
    }

    // As the GUI self-test profile answers the same
    void test_headlessUserWindowCommandLineInItsShowHandler()
    {
        QFETCH(const QString, line);
        QFETCH(const QString, during);
        QFETCH(const QString, expected);
        const QString hostname = qsl("Test-Headless-Host-User-Window-Command-Line-Show-Handler");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->registerAnonymousEventHandler(qsl("sysUserWindowResizeEvent"), qsl("onUwlShowHandler"));
        QFile::remove(headlessLayoutPath());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
uwlShowHandler = "not run"
local during, found = nil, "not run"
function onUwlShowHandler(_, _, _, name)
  if name == "uwl" and during then
    local act = during
    during = nil
    found = tostring(act())
  end
end
local ok, err = pcall(function()
  assert(saveWindowLayout())
  openUserWindow("uwl", false, false, "f")
  resizeWindow("uwl", 400, 250)
  %1
  createMiniConsole("uwl", "uwlOld", 0, 0, 50, 50)
  createScrollBox("uwl", "uwlBox", 0, 0, 80, 80)
  hideWindow("uwlBox")
  hideWindow("uwl")
  resizeWindow("uwl", 360, 220)
  during = function() %2 end
  local shown = showWindow("uwl")
  uwlShowHandler = found .. "|" .. tostring(shown) .. "|" .. tostring(windowVisible("uwl"))
end)
if not ok then uwlShowHandler = tostring(err) end
)lua")
                                                                           .arg(line, during)));
        const QString answer = luaGlobalString(host, "uwlShowHandler");
        // Before comparing, so a failing row leaves the next its profile name
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(answer, expected);
        QVERIFY(QFile::remove(headlessLayoutPath()));
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessWindowSizesContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Sizes");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessWindowSizes = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function look(label, name)
    answers[#answers + 1] = label .. ":geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- QWIDGETSIZE_MAX is 16777215
  local huge = 20000000
  createLabel("szNegativeLabel", 1, 2, -5, -7, 1)
  look("negativeLabel", "szNegativeLabel")
  createLabel("szHugeLabel", 1, 2, huge, huge, 1)
  look("hugeLabel", "szHugeLabel")
  createMiniConsole("szNegativeMini", 1, 2, -5, -7)
  look("negativeMini", "szNegativeMini")
  createMiniConsole("szHugeMini", 1, 2, huge, huge)
  look("hugeMini", "szHugeMini")
  createScrollBox("szNegativeBox", 1, 2, -5, -7)
  look("negativeBox", "szNegativeBox")
  createScrollBox("szHugeBox", 1, 2, huge, huge)
  look("hugeBox", "szHugeBox")
  for _, name in ipairs({"szNegativeLabel", "szNegativeMini", "szNegativeBox"}) do
    resizeWindow(name, huge, huge)
    look(name .. ":resizedHuge", name)
    resizeWindow(name, -3, -4)
    look(name .. ":resizedNegative", name)
  end
  -- A scroll box is bounded only as it is made
  createScrollBox("szNegativeBox", 1, 2, -6, -8)
  look("negativeBox:remadeNegative", "szNegativeBox")
  createScrollBox("szHugeBox", 1, 2, huge, huge)
  look("hugeBox:remadeHuge", "szHugeBox")
  -- A label made where nothing has been shown, then moved out with show false
  openUserWindow("szHidden", false, false, "f")
  hideWindow("szHidden")
  createScrollBox("szHidden", "szHiddenBox", 0, 0, 50, 50)
  resizeWindow("szHiddenBox", -3, -4)
  look("hiddenBox:resizedNegative", "szHiddenBox")
  createLabel("szHiddenBox", "szNeverShown", 1, 1, 20, 20, 1)
  look("neverShown", "szNeverShown")
  setWindow("main", "szNeverShown", 3, 3, false)
  look("neverShownMoved", "szNeverShown")
  for _, name in ipairs({"szNegativeLabel", "szHugeLabel", "szNeverShown"}) do
    deleteLabel(name)
  end
  for _, name in ipairs({"szNegativeMini", "szHugeMini"}) do
    deleteMiniConsole(name)
  end
  for _, name in ipairs({"szNegativeBox", "szHugeBox", "szHiddenBox"}) do
    deleteScrollBox(name)
  end
  closeUserWindow("szHidden")
  headlessWindowSizesAnswers = table.concat(answers, "\n")
end)
headlessWindowSizes = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessWindowSizes"), qsl("ok"));
        const QStringList answers{qsl("negativeLabel:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("hugeLabel:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("negativeMini:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("hugeMini:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("negativeBox:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("hugeBox:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("szNegativeLabel:resizedHuge:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("szNegativeLabel:resizedNegative:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("szNegativeMini:resizedHuge:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("szNegativeMini:resizedNegative:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("szNegativeBox:resizedHuge:geometry=[4]1:2:20000000:20000000,visible=[1]true"),
                                  qsl("szNegativeBox:resizedNegative:geometry=[4]1:2:-3:-4,visible=[1]true"),
                                  qsl("negativeBox:remadeNegative:geometry=[4]1:2:-6:-8,visible=[1]true"),
                                  qsl("hugeBox:remadeHuge:geometry=[4]1:2:20000000:20000000,visible=[1]true"),
                                  qsl("hiddenBox:resizedNegative:geometry=[4]0:0:-3:-4,visible=[1]false"),
                                  qsl("neverShown:geometry=[4]1:1:20:20,visible=[1]false"),
                                  qsl("neverShownMoved:geometry=[4]3:3:20:20,visible=[1]false")};
        QCOMPARE(luaGlobalString(host, "headlessWindowSizesAnswers"), answers.join(QLatin1Char('\n')));
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones,
    // which a spec cannot wait for: what was moved into a deleted scroll box goes with it
    void test_headlessSetWindowContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Set-Window");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSetWindow = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- A label measures its text in the font it has, which is read against labels made in the main window and in a user window
  local function font(label, name)
    local width, height = getLabelSizeHint(name)
    local kinds = {}
    for _, parent in ipairs({"main", "swWindow"}) do
      createLabel(parent, "swTwin", 0, 0, 50, 20, 1)
      echo("swTwin", getLabelText(name))
      local twinWidth, twinHeight = getLabelSizeHint("swTwin")
      deleteLabel("swTwin")
      if width == twinWidth and height == twinHeight then
        kinds[#kinds + 1] = parent
      end
    end
    answers[#answers + 1] = label .. ":font=" .. (#kinds > 0 and table.concat(kinds, "+") or "other")
  end
  assert(openUserWindow("swWindow", false, false, "f") == true, "openUserWindow did not answer true")
  setFont("swWindow", "Bitstream Vera Sans Mono")
  setFontSize("swWindow", 20)
  assert(createScrollBox("swBox", 10, 20, 200, 100))
  assert(createScrollBox("swWindow", "swWinBox", 3, 4, 150, 80))
  assert(createScrollBox("swBox", "swInner", 5, 6, 60, 50))
  createLabel("swInner", "swDeep", 1, 2, 30, 15, 1)
  echo("swDeep", "deep")
  createLabel("swInner", "swDeepPlain", 1, 20, 40, 20, 1)
  setLabelStyleSheet("swDeepPlain", "")
  echo("swDeepPlain", "words")

  createLabel("main", "swLabel", 7, 8, 40, 20, 1)
  echo("swLabel", "words")
  font("label", "swLabel")
  step("labelIntoBox", setWindow, "swBox", "swLabel", 3, 3, true)
  look("labelInBox", "swLabel")
  font("labelInBox", "swLabel")
  hideWindow("swBox")
  look("labelInHiddenBox", "swLabel")
  showWindow("swBox")
  step("labelIntoWinBox", setWindow, "swWinBox", "swLabel", 4, 4, false)
  look("labelInWinBox", "swLabel")
  font("labelInWinBox", "swLabel")
  step("showLabel", showWindow, "swLabel")
  look("labelShown", "swLabel")
  hideWindow("swWindow")
  look("labelInHiddenWindow", "swLabel")
  showWindow("swWindow")
  step("labelIntoInner", setWindow, "swInner", "swLabel", 1, 1, true)
  hideWindow("swBox")
  look("labelInHiddenOuter", "swLabel")
  step("labelIntoHiddenBox", setWindow, "swBox", "swLabel", 2, 2, true)
  look("labelIntoHidden", "swLabel")
  showWindow("swBox")
  look("labelOutOfHidden", "swLabel")
  step("labelIntoWindow", setWindow, "swWindow", "swLabel", 5, 5, true)
  look("labelInWindow", "swLabel")
  font("labelInWindow", "swLabel")
  step("labelIntoMain", setWindow, "main", "swLabel", 6, 6, true)
  look("labelInMain", "swLabel")
  font("labelInMain", "swLabel")
  -- QWidget::setParent(QWidget*) leaves a widget whose parent stays the same shown or hidden
  step("labelSameHidden", setWindow, "main", "swLabel", 6, 6, false)
  look("labelSameHidden", "swLabel")
  hideWindow("swLabel")
  step("hiddenLabelSame", setWindow, "main", "swLabel", 6, 6, false)
  look("hiddenLabelSame", "swLabel")
  step("labelSameShown", setWindow, "main", "swLabel", 6, 6, true)
  look("labelSameShown", "swLabel")

  createLabel("main", "swPlain", 0, 0, 40, 20, 1)
  setLabelStyleSheet("swPlain", "")
  echo("swPlain", "words")
  font("plain", "swPlain")
  step("plainIntoWinBox", setWindow, "swWinBox", "swPlain", 0, 0, true)
  font("plainInWinBox", "swPlain")
  step("plainIntoBox", setWindow, "swBox", "swPlain", 0, 0, true)
  font("plainInBox", "swPlain")
  step("plainOutToWinBox", setWindow, "swWinBox", "swPlain", 1, 1, true)

  createMiniConsole("main", "swMini", 9, 10, 50, 40)
  step("miniIntoBox", setWindow, "swBox", "swMini", 5, 5, true)
  look("miniInBox", "swMini")
  step("miniFontSize", getFontSize, "swMini")
  hideWindow("swBox")
  look("miniInHiddenBox", "swMini")
  showWindow("swBox")
  step("miniIntoWinBox", setWindow, "swWinBox", "swMini", 6, 6, false)
  look("miniInWinBox", "swMini")
  step("showMini", showWindow, "swMini")
  look("miniShown", "swMini")
  step("miniIntoMain", setWindow, "main", "swMini", 2, 2, true)
  look("miniInMain", "swMini")
  step("miniIntoWindow", setWindow, "swWindow", "swMini", 1, 1, true)
  hideWindow("swWindow")
  look("miniInHiddenWindow", "swMini")
  showWindow("swWindow")
  step("miniWinFontSize", getFontSize, "swMini")
  step("miniSameHidden", setWindow, "swWindow", "swMini", 1, 1, false)
  look("miniSameHidden", "swMini")
  hideWindow("swMini")
  step("hiddenMiniSame", setWindow, "swWindow", "swMini", 1, 1, false)
  look("hiddenMiniSame", "swMini")
  step("miniSameShown", setWindow, "swWindow", "swMini", 1, 1, true)
  look("miniSameShown", "swMini")

  step("boxIntoWindow", setWindow, "swWindow", "swBox", 6, 6, true)
  look("boxInWindow", "swBox")
  font("deepInWindowBox", "swDeep")
  font("deepPlainInWindowBox", "swDeepPlain")
  hideWindow("swWindow")
  look("boxInHiddenWindow", "swBox")
  look("deepInHiddenWindow", "swDeep")
  showWindow("swWindow")
  step("boxSameHidden", setWindow, "swWindow", "swBox", 6, 6, false)
  look("boxSameHidden", "swBox")
  look("deepInSameHidden", "swDeep")
  hideWindow("swBox")
  step("hiddenBoxSame", setWindow, "swWindow", "swBox", 6, 6, false)
  look("hiddenBoxSame", "swBox")
  look("deepInHiddenBoxSame", "swDeep")
  step("boxSameShown", setWindow, "swWindow", "swBox", 6, 6, true)
  look("boxSameShown", "swBox")
  look("deepInSameShown", "swDeep")
  step("boxIntoItself", setWindow, "swBox", "swBox", 0, 0, true)
  step("boxIntoChild", setWindow, "swInner", "swBox", 0, 0, true)
  step("boxIntoMissing", setWindow, "swNoSuch", "swBox", 0, 0, true)
  step("missingIntoBox", setWindow, "swBox", "swNoSuch", 0, 0, true)
  step("windowIntoBox", setWindow, "swBox", "swWindow", 0, 0, true)
  step("mapperIntoBox", setWindow, "swBox", "mapper", 0, 0, true)
  step("mainIntoBox", setWindow, "swBox", "main", 0, 0, true)
  step("innerToMainHidden", setWindow, "main", "swInner", 7, 7, false)
  look("innerInMain", "swInner")
  look("deepInHiddenInner", "swDeep")
  step("innerBack", setWindow, "swBox", "swInner", 5, 6, true)
  look("innerBack", "swInner")
  look("deepBack", "swDeep")
  step("innerSameHidden", setWindow, "swBox", "swInner", 5, 6, false)
  look("innerSameHidden", "swInner")
  hideWindow("swInner")
  step("hiddenInnerSame", setWindow, "swBox", "swInner", 5, 6, false)
  look("hiddenInnerSame", "swInner")
  step("innerSameShown", setWindow, "swBox", "swInner", 5, 6, true)
  look("innerSameShown", "swInner")
  step("boxIntoMain", setWindow, "main", "swBox", 11, 12, true)
  look("boxInMain", "swBox")
  step("boxIntoWinBox", setWindow, "swWinBox", "swBox", 1, 1, true)
  hideWindow("swWindow")
  look("boxInHiddenWinBox", "swBox")
  look("deepInHiddenWinBox", "swDeep")
  showWindow("swWindow")
  font("deepPlainInWinBox", "swDeepPlain")
  step("boxBackToMain", setWindow, "main", "swBox", 11, 12, true)
  font("deepPlainInMain", "swDeepPlain")

  -- setWindow takes a scroll box before a user window of the same name
  assert(createScrollBox("swWindow", 30, 30, 20, 20))
  step("plainIntoClash", setWindow, "swWindow", "swPlain", 0, 0, true)
  font("plainInClash", "swPlain")
  step("plainOutOfClash", setWindow, "swWinBox", "swPlain", 1, 1, true)
  deleteScrollBox("swWindow")
  step("miniIntoBoxAgain", setWindow, "swBox", "swMini", 5, 5, true)
  step("labelIntoInnerAgain", setWindow, "swInner", "swLabel", 1, 1, true)
  step("delete", deleteScrollBox, "swBox")
  look("labelAtOnce", "swLabel")
  look("miniAtOnce", "swMini")
  look("plainOutside", "swPlain")
  -- Made while their user window is hidden
  assert(openUserWindow("swHidden", false, false, "f") == true, "openUserWindow did not answer true")
  hideWindow("swHidden")
  createMiniConsole("swHidden", "swHiddenMini", 0, 0, 20, 20)
  assert(createScrollBox("swHidden", "swHiddenBox", 0, 30, 20, 20))
  look("hiddenMiniMade", "swHiddenMini")
  step("hiddenMiniSameHidden", setWindow, "swHidden", "swHiddenMini", 0, 0, false)
  step("hiddenBoxSameShown", setWindow, "swHidden", "swHiddenBox", 0, 30, true)
  showWindow("swHidden")
  look("hiddenMiniSameHidden", "swHiddenMini")
  look("hiddenBoxSameShown", "swHiddenBox")
  hideWindow("swHidden")
  step("hiddenMiniSameShown", setWindow, "swHidden", "swHiddenMini", 0, 0, true)
  step("hiddenBoxSameHidden", setWindow, "swHidden", "swHiddenBox", 0, 30, false)
  showWindow("swHidden")
  look("hiddenMiniSameShown", "swHiddenMini")
  look("hiddenBoxSameHidden", "swHiddenBox")
  closeUserWindow("swHidden")
  closeUserWindow("swWindow")
  headlessSetWindowAnswers = table.concat(answers, "\n")
end)
headlessSetWindow = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessSetWindowAfter()
  local answers = {}
  for _, name in ipairs({"swLabel", "swMini", "swDeep", "swInner", "swPlain", "swWinBox", "swDeepPlain"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSetWindow"), qsl("ok"));
        const QStringList answers{qsl("label:font=main"),
qsl("labelIntoBox=[1]true"),
qsl("labelInBox:type=[1]label,geometry=[4]3:3:40:20,visible=[1]true"),
qsl("labelInBox:font=other"),
qsl("labelInHiddenBox:type=[1]label,geometry=[4]3:3:40:20,visible=[1]false"),
qsl("labelIntoWinBox=[1]true"),
qsl("labelInWinBox:type=[1]label,geometry=[4]4:4:40:20,visible=[1]false"),
qsl("labelInWinBox:font=other"),
qsl("showLabel=[1]true"),
qsl("labelShown:type=[1]label,geometry=[4]4:4:40:20,visible=[1]true"),
qsl("labelInHiddenWindow:type=[1]label,geometry=[4]4:4:40:20,visible=[1]false"),
qsl("labelIntoInner=[1]true"),
qsl("labelInHiddenOuter:type=[1]label,geometry=[4]1:1:40:20,visible=[1]false"),
qsl("labelIntoHiddenBox=[1]true"),
qsl("labelIntoHidden:type=[1]label,geometry=[4]2:2:40:20,visible=[1]false"),
qsl("labelOutOfHidden:type=[1]label,geometry=[4]2:2:40:20,visible=[1]true"),
qsl("labelIntoWindow=[1]true"),
qsl("labelInWindow:type=[1]label,geometry=[4]5:5:40:20,visible=[1]true"),
qsl("labelInWindow:font=other"),
qsl("labelIntoMain=[1]true"),
qsl("labelInMain:type=[1]label,geometry=[4]6:6:40:20,visible=[1]true"),
qsl("labelInMain:font=other"),
qsl("labelSameHidden=[1]true"),
qsl("labelSameHidden:type=[1]label,geometry=[4]6:6:40:20,visible=[1]true"),
qsl("hiddenLabelSame=[1]true"),
qsl("hiddenLabelSame:type=[1]label,geometry=[4]6:6:40:20,visible=[1]false"),
qsl("labelSameShown=[1]true"),
qsl("labelSameShown:type=[1]label,geometry=[4]6:6:40:20,visible=[1]true"),
qsl("plain:font=other"),
qsl("plainIntoWinBox=[1]true"),
qsl("plainInWinBox:font=swWindow"),
qsl("plainIntoBox=[1]true"),
qsl("plainInBox:font=main"),
qsl("plainOutToWinBox=[1]true"),
qsl("miniIntoBox=[1]true"),
qsl("miniInBox:type=[1]miniconsole,geometry=[4]5:5:50:40,visible=[1]true"),
qsl("miniFontSize=[1]12"),
qsl("miniInHiddenBox:type=[1]miniconsole,geometry=[4]5:5:50:40,visible=[1]false"),
qsl("miniIntoWinBox=[1]true"),
qsl("miniInWinBox:type=[1]miniconsole,geometry=[4]6:6:50:40,visible=[1]false"),
qsl("showMini=[1]true"),
qsl("miniShown:type=[1]miniconsole,geometry=[4]6:6:50:40,visible=[1]true"),
qsl("miniIntoMain=[1]true"),
qsl("miniInMain:type=[1]miniconsole,geometry=[4]2:2:50:40,visible=[1]true"),
qsl("miniIntoWindow=[1]true"),
qsl("miniInHiddenWindow:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]false"),
qsl("miniWinFontSize=[1]12"),
qsl("miniSameHidden=[1]true"),
qsl("miniSameHidden:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]true"),
qsl("hiddenMiniSame=[1]true"),
qsl("hiddenMiniSame:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]false"),
qsl("miniSameShown=[1]true"),
qsl("miniSameShown:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]true"),
qsl("boxIntoWindow=[1]true"),
qsl("boxInWindow:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]true"),
qsl("deepInWindowBox:font=main"),
qsl("deepPlainInWindowBox:font=swWindow"),
qsl("boxInHiddenWindow:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]false"),
qsl("deepInHiddenWindow:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("boxSameHidden=[1]true"),
qsl("boxSameHidden:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]true"),
qsl("deepInSameHidden:type=[1]label,geometry=[4]1:2:30:15,visible=[1]true"),
qsl("hiddenBoxSame=[1]true"),
qsl("hiddenBoxSame:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]false"),
qsl("deepInHiddenBoxSame:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("boxSameShown=[1]true"),
qsl("boxSameShown:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]true"),
qsl("deepInSameShown:type=[1]label,geometry=[4]1:2:30:15,visible=[1]true"),
qsl("boxIntoItself=[2]nil:element 'swBox' cannot be moved into itself or into one of its own children"),
qsl("boxIntoChild=[2]nil:element 'swBox' cannot be moved into itself or into one of its own children"),
qsl("boxIntoMissing=[2]nil:window 'swNoSuch' not found"),
qsl("missingIntoBox=[2]nil:element 'swNoSuch' not found"),
qsl("windowIntoBox=[2]nil:element 'swWindow' is the base of a floating/dockable user window and may not be moved"),
qsl("mapperIntoBox=[2]nil:element 'mapper' not found"),
qsl("mainIntoBox=[2]nil:element 'main' not found"),
qsl("innerToMainHidden=[1]true"),
qsl("innerInMain:type=[1]scrollbox,geometry=[4]7:7:60:50,visible=[1]false"),
qsl("deepInHiddenInner:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("innerBack=[1]true"),
qsl("innerBack:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
qsl("deepBack:type=[1]label,geometry=[4]1:2:30:15,visible=[1]true"),
qsl("innerSameHidden=[1]true"),
qsl("innerSameHidden:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
qsl("hiddenInnerSame=[1]true"),
qsl("hiddenInnerSame:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]false"),
qsl("innerSameShown=[1]true"),
qsl("innerSameShown:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
qsl("boxIntoMain=[1]true"),
qsl("boxInMain:type=[1]scrollbox,geometry=[4]11:12:200:100,visible=[1]true"),
qsl("boxIntoWinBox=[1]true"),
qsl("boxInHiddenWinBox:type=[1]scrollbox,geometry=[4]1:1:200:100,visible=[1]false"),
qsl("deepInHiddenWinBox:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("deepPlainInWinBox:font=swWindow"),
qsl("boxBackToMain=[1]true"),
qsl("deepPlainInMain:font=main"),
qsl("plainIntoClash=[1]true"),
qsl("plainInClash:font=main"),
qsl("plainOutOfClash=[1]true"),
qsl("miniIntoBoxAgain=[1]true"),
qsl("labelIntoInnerAgain=[1]true"),
qsl("delete=[1]true"),
qsl("labelAtOnce:type=[1]label,geometry=[4]1:1:40:20,visible=[1]true"),
qsl("miniAtOnce:type=[1]miniconsole,geometry=[4]5:5:50:40,visible=[1]true"),
qsl("plainOutside:type=[1]label,geometry=[4]1:1:40:20,visible=[1]true"),
qsl("hiddenMiniMade:type=[1]miniconsole,geometry=[4]0:0:20:20,visible=[1]false"),
qsl("hiddenMiniSameHidden=[1]true"),
qsl("hiddenBoxSameShown=[1]true"),
qsl("hiddenMiniSameHidden:type=[1]miniconsole,geometry=[4]0:0:20:20,visible=[1]true"),
qsl("hiddenBoxSameShown:type=[1]scrollbox,geometry=[4]0:30:20:20,visible=[1]true"),
qsl("hiddenMiniSameShown=[1]true"),
qsl("hiddenBoxSameHidden=[1]true"),
qsl("hiddenMiniSameShown:type=[1]miniconsole,geometry=[4]0:0:20:20,visible=[1]true"),
qsl("hiddenBoxSameHidden:type=[1]scrollbox,geometry=[4]0:30:20:20,visible=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessSetWindowAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSetWindowAfterAnswers = headlessSetWindowAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{qsl("swLabel:nil"), qsl("swMini:nil"), qsl("swDeep:nil"), qsl("swInner:nil"), qsl("swPlain:label"), qsl("swWinBox:scrollbox"), qsl("swDeepPlain:nil")};
        QCOMPARE(luaGlobalString(host, "headlessSetWindowAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Moving the windows created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones,
    // which a spec cannot wait for: a command line in a deleted scroll box goes with it
    void test_headlessCommandLineContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Command-Line");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLine = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      -- Bytes a line of answers cannot hold as they are, by number
      values[i] = tostring((select(i, ...))):gsub("[%c\128-\255]", function(byte)
        return "\\" .. byte:byte()
      end)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  local deletedEvents = {}
  -- By name, as the C++ registerAnonymousEventHandler() takes no function
  function headlessCommandLineDeleted(_, name)
    deletedEvents[#deletedEvents + 1] = name
  end
  registerAnonymousEventHandler("sysCommandLineDeleted", "headlessCommandLineDeleted")
  local function events(label)
    answers[#answers + 1] = label .. ":events=" .. table.concat(deletedEvents, ",")
    deletedEvents = {}
  end
  step("create", createCommandLine, "clMain", 10, 20, 200, 30)
  look("created", "clMain")
  step("createAgain", createCommandLine, "clMain", 0, 0, 10, 10)
  step("createEmpty", createCommandLine, "", 0, 0, 10, 10)
  step("createInMissing", createCommandLine, "clNoSuch", "clLost", 0, 0, 10, 10)
  look("lost", "clLost")
  step("createInMain", createCommandLine, "main", "clInMain", 1, 2, 30, 40)
  look("inMain", "clInMain")
  step("createNegative", createCommandLine, "clNegative", 3, 4, -5, -6)
  look("negative", "clNegative")
  step("createBadName", createCommandLine, {}, 0, 0, 10, 10)
  step("createBadParent", createCommandLine, {}, "clBad", 0, 0, 10, 10)
  step("createBadX", createCommandLine, "clBad", "x", 0, 10, 10)
  step("createBadHeight", createCommandLine, "main", "clBad", 0, 0, 10, "h")
  look("bad", "clBad")

  step("getNew", getCmdLine, "clMain")
  step("print", printCmdLine, "clMain", "hello")
  step("getPrinted", getCmdLine, "clMain")
  step("append", appendCmdLine, "clMain", " world")
  step("getAppended", getCmdLine, "clMain")
  step("printOdd", printCmdLine, "clMain", "a\tb\r\nc\rd\194\160e\226\128\168f\226\128\169g\0h")
  step("getOdd", getCmdLine, "clMain")
  step("appendOdd", appendCmdLine, "clMain", "\194\160x\n")
  step("getAppendedOdd", getCmdLine, "clMain")
  step("printNumber", printCmdLine, "clMain", 42)
  step("getNumber", getCmdLine, "clMain")
  step("clear", clearCmdLine, "clMain")
  step("getCleared", getCmdLine, "clMain")
  step("appendToEmpty", appendCmdLine, "clMain", "x")
  step("getAppendedToEmpty", getCmdLine, "clMain")
  look("afterText", "clMain")
  step("printMissing", printCmdLine, "clNoSuch", "x")
  step("appendMissing", appendCmdLine, "clNoSuch", "x")
  step("clearMissing", clearCmdLine, "clNoSuch")
  step("getMissing", getCmdLine, "clNoSuch")
  step("printBadName", printCmdLine, {}, "x")
  step("printBadText", printCmdLine, "clMain", {})
  step("getBadName", getCmdLine, {})
  step("getOther", getCmdLine, "clInMain")

  step("action", setCmdLineAction, "clMain", function() end)
  step("actionAgain", setCmdLineAction, "clMain", function() end)
  step("actionMissing", setCmdLineAction, "clNoSuch", function() end)
  step("actionEmpty", setCmdLineAction, "", function() end)
  step("actionMain", setCmdLineAction, "main", function() end)
  step("actionBadName", setCmdLineAction, {}, function() end)
  step("reset", resetCmdLineAction, "clMain")
  step("resetAgain", resetCmdLineAction, "clMain")
  step("resetMissing", resetCmdLineAction, "clNoSuch")
  step("resetEmpty", resetCmdLineAction, "")
  step("resetMain", resetCmdLineAction, "main")

  step("raise", raiseWindow, "clMain")
  step("lower", lowerWindow, "clMain")
  step("hide", hideWindow, "clMain")
  look("hidden", "clMain")
  step("show", showWindow, "clMain")
  look("shown", "clMain")
  step("move", moveWindow, "clMain", 5, 6)
  step("resize", resizeWindow, "clMain", 70, 25)
  look("moved", "clMain")
  step("resizeNegative", resizeWindow, "clMain", -1, -2)
  look("resizedNegative", "clMain")
  step("resizeBack", resizeWindow, "clMain", 70, 25)

  assert(openUserWindow("clWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("createInWindow", createCommandLine, "clWindow", "clWin", 1, 1, 50, 20)
  look("inWindow", "clWin")
  hideWindow("clWindow")
  look("inHiddenWindow", "clWin")
  showWindow("clWindow")
  look("inShownWindow", "clWin")
  assert(createScrollBox("clWindow", "clBox", 0, 30, 100, 50))
  step("createInBox", createCommandLine, "clBox", "clBoxed", 2, 2, 40, 20)
  look("inBox", "clBoxed")
  hideWindow("clBox")
  look("inHiddenBox", "clBoxed")
  step("createInHiddenBox", createCommandLine, "clBox", "clLate", 2, 25, 40, 20)
  look("madeInHiddenBox", "clLate")
  step("showInHiddenBox", showWindow, "clLate")
  look("shownInHiddenBox", "clLate")
  showWindow("clBox")
  step("hideBoxed", hideWindow, "clBoxed")
  hideWindow("clBox")
  showWindow("clBox")
  look("boxedStaysHidden", "clBoxed")
  showWindow("clBoxed")
  step("textInBox", printCmdLine, "clBoxed", "boxed")
  step("getInBox", getCmdLine, "clBoxed")

  step("intoBox", setWindow, "clBox", "clMain", 3, 3, true)
  look("movedIntoBox", "clMain")
  hideWindow("clWindow")
  look("movedInHiddenWindow", "clMain")
  showWindow("clWindow")
  step("intoMainHidden", setWindow, "main", "clMain", 4, 4, false)
  look("movedToMainHidden", "clMain")
  step("showMoved", showWindow, "clMain")
  step("sameParentHidden", setWindow, "main", "clMain", 7, 8, false)
  look("sameParent", "clMain")
  step("intoWindow", setWindow, "clWindow", "clMain", 9, 9, true)
  look("movedIntoWindow", "clMain")
  step("textAfterMove", getCmdLine, "clMain")
  step("intoMain", setWindow, "main", "clMain", 10, 20, true)
  -- A scroll box takes a command line before a user window of the same name
  assert(openUserWindow("clTwin", false, false, "f") == true, "openUserWindow did not answer true")
  assert(createScrollBox("clTwin", 0, 0, 100, 50))
  step("createInTwin", createCommandLine, "clTwin", "clInTwin", 1, 1, 40, 20)
  hideWindow("clTwin")
  look("twinWindowHidden", "clInTwin")
  showWindow("clTwin")
  -- What a scroll box holds moves with it
  assert(createScrollBox("clWindow", "clMover", 0, 0, 100, 50))
  step("createInMover", createCommandLine, "clMover", "clRider", 1, 1, 40, 20)
  step("moverIntoMain", setWindow, "main", "clMover", 0, 0, true)
  hideWindow("clWindow")
  look("riderAfterMove", "clRider")
  showWindow("clWindow")
  deleteScrollBox("clTwin")
  deleteScrollBox("clMover")

  -- A command line may share a scroll box's name, and what is read for the name is the box's while there is one
  assert(createScrollBox("clShared", 1, 2, 100, 100))
  hideWindow("clShared")
  step("lineAfterBox", createCommandLine, "clShared", 5, 6, 40, 20)
  look("boxThenLine", "clShared")
  step("resizeShared", resizeWindow, "clShared", 60, 70)
  look("sharedResized", "clShared")
  step("deleteSharedLine", deleteCommandLine, "clShared")
  look("sharedLineDeleted", "clShared")
  deleteScrollBox("clShared")
  step("lineBeforeBox", createCommandLine, "clSharedLater", 5, 6, 40, 20)
  moveWindow("clSharedLater", 9, 9)
  resizeWindow("clSharedLater", 45, 25)
  step("boxAfterLine", createScrollBox, "clSharedLater", 1, 2, 100, 100)
  look("lineThenBox", "clSharedLater")
  hideWindow("clSharedLater")
  look("sharedHidden", "clSharedLater")
  step("deleteSharedBox", deleteScrollBox, "clSharedLater")
  look("sharedBoxDeleted", "clSharedLater")
  moveWindow("clSharedLater", 7, 8)
  look("sharedLineMoved", "clSharedLater")
  deleteCommandLine("clSharedLater")
  createCommandLine("clSharedMoved", 5, 6, 40, 20)
  moveWindow("clSharedMoved", 9, 9)
  createScrollBox("clSharedMoved", 1, 2, 100, 100)
  deleteScrollBox("clSharedMoved")
  look("movedBeforeBox", "clSharedMoved")
  deleteCommandLine("clSharedMoved")

  step("delete", deleteCommandLine, "clInMain")
  events("delete")
  look("deleted", "clInMain")
  step("getDeleted", getCmdLine, "clInMain")
  step("printDeleted", printCmdLine, "clInMain", "x")
  step("actionDeleted", setCmdLineAction, "clInMain", function() end)
  step("raiseDeleted", raiseWindow, "clInMain")
  step("deleteAgain", deleteCommandLine, "clInMain")
  step("deleteMain", deleteCommandLine, "main")
  step("deleteEmpty", deleteCommandLine, "")
  step("deleteMissing", deleteCommandLine, "clNoSuch")
  step("deleteBadName", deleteCommandLine, {})
  events("failedDeletes")
  step("recreate", createCommandLine, "clInMain", 1, 2, 30, 40)
  step("getRecreated", getCmdLine, "clInMain")
  step("deleteRecreated", deleteCommandLine, "clInMain")
  events("recreated")

  step("deleteBox", deleteScrollBox, "clBox")
  events("box")
  look("boxedAtOnce", "clBoxed")
  step("deleteNegative", deleteCommandLine, "clNegative")
  step("deleteFirst", deleteCommandLine, "clMain")
  events("end")
  -- A box in a box gives up its name when the outer one goes
  assert(createScrollBox("clOuter", 0, 0, 100, 100))
  assert(createScrollBox("clOuter", "clNested", 0, 0, 50, 50))
  hideWindow("clNested")
  step("lineNamedForNested", createCommandLine, "clNested", 11, 12, 40, 20)
  deleteScrollBox("clOuter")
  closeUserWindow("clWindow")
  closeUserWindow("clTwin")
  headlessCommandLineAnswers = table.concat(answers, "\n")
end)
headlessCommandLine = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessCommandLineAfter()
  local answers = {}
  for _, name in ipairs({"clMain", "clInMain", "clWin", "clBoxed", "clBox", "clWindow"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  answers[#answers + 1] = "clNested:" .. tostring(windowType("clNested")) .. ":" .. table.concat({getWindowGeometry("clNested")}, ",") .. ":" .. tostring(windowVisible("clNested"))
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLine"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("created:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]true"),
                                  qsl("createAgain=[2]false:couldn't create commandLine"),
                                  qsl("createEmpty=[2]false:a commandLine cannot have an empty string as its name"),
                                  qsl("createInMissing=[2]false:window 'clNoSuch' not found"),
                                  qsl("lost:type=[2]nil:'clLost' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"clLost\" not "
                                      "found,visible=[2]nil:window \"clLost\" not found"),
                                  qsl("createInMain=[1]true"),
                                  qsl("inMain:type=[1]commandline,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createNegative=[1]true"),
                                  qsl("negative:type=[1]commandline,geometry=[4]3:4:0:0,visible=[1]true"),
                                  qsl("createBadName=raised:createCommandLine: bad argument #1 type (commandLine name as string expected, got table!)"),
                                  qsl("createBadParent=raised:createCommandLine: bad argument #1 type (parent window name as string expected, got table!)"),
                                  qsl("createBadX=raised:createCommandLine: bad argument #2 type (commandline x-coordinate as number expected, got string!)"),
                                  qsl("createBadHeight=raised:createCommandLine: bad argument #6 type (commandline height as number expected, got string!)"),
                                  qsl("bad:type=[2]nil:'clBad' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"clBad\" not "
                                      "found,visible=[2]nil:window \"clBad\" not found"),
                                  qsl("getNew=[1]"),
                                  qsl("print=[0]"),
                                  qsl("getPrinted=[1]hello"),
                                  qsl("append=[0]"),
                                  qsl("getAppended=[1]hello world"),
                                  qsl("printOdd=[0]"),
                                  qsl("getOdd=[1]a\\9b\\10c\\10d e\\10f\\10g"),
                                  qsl("appendOdd=[0]"),
                                  qsl("getAppendedOdd=[1]a\\9b\\10c\\10d e\\10f\\10g x\\10"),
                                  qsl("printNumber=[0]"),
                                  qsl("getNumber=[1]42"),
                                  qsl("clear=[0]"),
                                  qsl("getCleared=[1]"),
                                  qsl("appendToEmpty=[0]"),
                                  qsl("getAppendedToEmpty=[1]x"),
                                  qsl("afterText:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]true"),
                                  qsl("printMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("appendMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("clearMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("getMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("printBadName=raised:printCmdLine: bad argument #1 type (command line name as string expected, got table)!"),
                                  qsl("printBadText=raised:printCmdLine: bad argument #2 type (text to set on command line as string expected, got table!)"),
                                  qsl("getBadName=raised:getCmdLine: bad argument #1 type (command line name as string expected, got table)!"),
                                  qsl("getOther=[1]"),
                                  qsl("action=[1]true"),
                                  qsl("actionAgain=[1]true"),
                                  qsl("actionMissing=[2]nil:command line name 'clNoSuch' not found"),
                                  qsl("actionEmpty=[2]nil:command line name cannot be an empty string"),
                                  qsl("actionMain=[2]nil:the main command line's action cannot be changed, use an alias to act on what is typed there"),
                                  qsl("actionBadName=raised:setCmdLineAction: bad argument #1 type (command line name as string expected, got table!)"),
                                  qsl("reset=[1]true"),
                                  qsl("resetAgain=[1]true"),
                                  qsl("resetMissing=[2]nil:command line name 'clNoSuch' not found"),
                                  qsl("resetEmpty=[2]nil:command line name cannot be an empty string"),
                                  qsl("resetMain=[2]nil:the main command line's action cannot be changed, use an alias to act on what is typed there"),
                                  qsl("raise=[1]true"),
                                  qsl("lower=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("hidden:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]false"),
                                  qsl("show=[1]true"),
                                  qsl("shown:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]true"),
                                  qsl("move=[1]true"),
                                  qsl("resize=[1]true"),
                                  qsl("moved:type=[1]commandline,geometry=[4]5:6:70:25,visible=[1]true"),
                                  qsl("resizeNegative=[1]true"),
                                  qsl("resizedNegative:type=[1]commandline,geometry=[4]5:6:0:0,visible=[1]true"),
                                  qsl("resizeBack=[1]true"),
                                  qsl("createInWindow=[1]true"),
                                  qsl("inWindow:type=[1]commandline,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("inHiddenWindow:type=[1]commandline,geometry=[4]1:1:50:20,visible=[1]false"),
                                  qsl("inShownWindow:type=[1]commandline,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("createInBox=[1]true"),
                                  qsl("inBox:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("inHiddenBox:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("createInHiddenBox=[1]true"),
                                  qsl("madeInHiddenBox:type=[1]commandline,geometry=[4]2:25:40:20,visible=[1]false"),
                                  qsl("showInHiddenBox=[1]true"),
                                  qsl("shownInHiddenBox:type=[1]commandline,geometry=[4]2:25:40:20,visible=[1]false"),
                                  qsl("hideBoxed=[1]true"),
                                  qsl("boxedStaysHidden:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("textInBox=[0]"),
                                  qsl("getInBox=[1]boxed"),
                                  qsl("intoBox=[1]true"),
                                  qsl("movedIntoBox:type=[1]commandline,geometry=[4]3:3:70:25,visible=[1]true"),
                                  qsl("movedInHiddenWindow:type=[1]commandline,geometry=[4]3:3:70:25,visible=[1]false"),
                                  qsl("intoMainHidden=[1]true"),
                                  qsl("movedToMainHidden:type=[1]commandline,geometry=[4]4:4:70:25,visible=[1]false"),
                                  qsl("showMoved=[1]true"),
                                  qsl("sameParentHidden=[1]true"),
                                  qsl("sameParent:type=[1]commandline,geometry=[4]7:8:70:25,visible=[1]true"),
                                  qsl("intoWindow=[1]true"),
                                  qsl("movedIntoWindow:type=[1]commandline,geometry=[4]9:9:70:25,visible=[1]true"),
                                  qsl("textAfterMove=[1]x"),
                                  qsl("intoMain=[1]true"),
                                  qsl("createInTwin=[1]true"),
                                  qsl("twinWindowHidden:type=[1]commandline,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("createInMover=[1]true"),
                                  qsl("moverIntoMain=[1]true"),
                                  qsl("riderAfterMove:type=[1]commandline,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("lineAfterBox=[1]true"),
                                  qsl("boxThenLine:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("resizeShared=[1]true"),
                                  qsl("sharedResized:type=[1]scrollbox,geometry=[4]1:2:60:70,visible=[1]false"),
                                  qsl("deleteSharedLine=[1]true"),
                                  qsl("sharedLineDeleted:type=[1]scrollbox,geometry=[4]1:2:60:70,visible=[1]false"),
                                  qsl("lineBeforeBox=[1]true"),
                                  qsl("boxAfterLine=[1]true"),
                                  qsl("lineThenBox:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]true"),
                                  qsl("sharedHidden:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("deleteSharedBox=[1]true"),
                                  qsl("sharedBoxDeleted:type=[1]commandline,geometry=[4]9:9:45:25,visible=[1]true"),
                                  qsl("sharedLineMoved:type=[1]commandline,geometry=[4]7:8:45:25,visible=[1]true"),
                                  qsl("movedBeforeBox:type=[1]commandline,geometry=[4]9:9:40:20,visible=[1]true"),
                                  qsl("delete=[1]true"),
                                  qsl("delete:events=clShared,clSharedLater,clSharedMoved,clInMain"),
                                  qsl("deleted:type=[2]nil:'clInMain' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"clInMain\" not "
                                      "found,visible=[2]nil:window \"clInMain\" not found"),
                                  qsl("getDeleted=[2]nil:command line \"clInMain\" not found"),
                                  qsl("printDeleted=[2]nil:command line \"clInMain\" not found"),
                                  qsl("actionDeleted=[2]nil:command line name 'clInMain' not found"),
                                  qsl("raiseDeleted=[1]false"),
                                  qsl("deleteAgain=[2]false:command line name 'clInMain' not found"),
                                  qsl("deleteMain=[2]false:the main command line cannot be deleted"),
                                  qsl("deleteEmpty=[2]false:the main command line cannot be deleted"),
                                  qsl("deleteMissing=[2]false:command line name 'clNoSuch' not found"),
                                  qsl("deleteBadName=raised:deleteCommandLine: bad argument #1 type (command line name as string expected, got table!)"),
                                  qsl("failedDeletes:events="),
                                  qsl("recreate=[1]true"),
                                  qsl("getRecreated=[1]"),
                                  qsl("deleteRecreated=[1]true"),
                                  qsl("recreated:events=clInMain"),
                                  qsl("deleteBox=[1]true"),
                                  qsl("box:events="),
                                  qsl("boxedAtOnce:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("deleteNegative=[1]true"),
                                  qsl("deleteFirst=[1]true"),
                                  qsl("end:events=clNegative,clMain"),
                                  qsl("lineNamedForNested=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessCommandLineAfterAnswers = headlessCommandLineAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{
                qsl("clMain:nil"), qsl("clInMain:nil"), qsl("clWin:commandline"), qsl("clBoxed:nil"), qsl("clBox:nil"), qsl("clWindow:userwindow"), qsl("clNested:commandline:11,12,40,20:true")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the command lines created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessCommandLineSettingsContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Command-Line-Settings");
        // Opened rather than added, as saveProfile() refuses a profile that was never loaded
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->mLoadedOk, qPrintable(host->mProfileLoadError));

        // With no view, pumpEvents() takes Mudlet for shutting down and pumps nothing, so the waits for a save use the
        // pump it would
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State* L) -> int {
            static_cast<void>(EventLoopPump::pumpFor(static_cast<int>(lua_tointeger(L, 1))));
            return 0;
        });
        // Run from the event loop, as a script in the GUI is, so the saves it waits out deliver no deferred deletes
        bool ran = false;
        bool pendingDeleteHeld = false;
        QEventLoop loop;
        QTimer::singleShot(0, &loop, [&ran, &pendingDeleteHeld, &loop, host]() {
            // The rows rely on the waits for a save delivering no deferred delete posted here, as in the GUI
            QPointer<QObject> pPending = new QObject();
            pPending->deleteLater();
            ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineSettings = "not run"
-- A save still running refuses the next one until the event loop has finished it
function headlessCommandLineSave(...)
  local saved, message
  for _ = 1, 100 do
    saved, message = saveProfile(...)
    if saved or not tostring(message):find("a save is already in progress", 1, true) then
      break
    end
    pumpEvents(50)
  end
  if not saved then
    error("the profile was not saved: " .. tostring(message))
  end
  return saved
end
-- What a save wrote to profile.ini, once the event loop has written it out
function headlessCommandLineIni(name)
  pumpEvents(300)
  local file = io.open(getMudletHomeDir() .. "/profile.ini", "r")
  if not file then
    return nil
  end
  local text = file:read("*a")
  file:close()
  local key = "SaveHistory\\" .. name .. "="
  local at = text:find(key, 1, true)
  return at and text:sub(at + #key):match("^%a+")
end
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  local save = headlessCommandLineSave
  step("create", createCommandLine, "csLine", 10, 20, 200, 30)

  step("suggest", addCmdLineSuggestion, "csLine", "word")
  step("suggestAgain", addCmdLineSuggestion, "csLine", "word")
  step("unsuggest", removeCmdLineSuggestion, "csLine", "word")
  step("unsuggestAbsent", removeCmdLineSuggestion, "csLine", "absent")
  step("clearSuggestions", clearCmdLineSuggestions, "csLine")
  step("suggestMissing", addCmdLineSuggestion, "csNoSuch", "word")
  step("unsuggestMissing", removeCmdLineSuggestion, "csNoSuch", "word")
  step("clearSuggestionsMissing", clearCmdLineSuggestions, "csNoSuch")
  step("suggestEmpty", addCmdLineSuggestion, "csLine", "")
  step("suggestBadText", addCmdLineSuggestion, "csLine", {})

  step("blacklist", addCmdLineBlacklist, "csLine", "word")
  step("unblacklist", removeCmdLineBlacklist, "csLine", "word")
  step("unblacklistAbsent", removeCmdLineBlacklist, "csLine", "absent")
  step("clearBlacklist", clearCmdLineBlacklist, "csLine")
  step("blacklistMissing", addCmdLineBlacklist, "csNoSuch", "word")
  step("unblacklistMissing", removeCmdLineBlacklist, "csNoSuch", "word")
  step("clearBlacklistMissing", clearCmdLineBlacklist, "csNoSuch")

  step("menu", addCommandLineMenuEvent, "csLine", "Item", "csEvent")
  step("menuAgain", addCommandLineMenuEvent, "csLine", "Item", "csOtherEvent")
  step("menuOther", addCommandLineMenuEvent, "csLine", "Other", "csEvent")
  step("unmenu", removeCommandLineMenuEvent, "csLine", "Item")
  step("unmenuAgain", removeCommandLineMenuEvent, "csLine", "Item")
  step("unmenuOther", removeCommandLineMenuEvent, "csLine", "Other")
  step("menuMissing", addCommandLineMenuEvent, "csNoSuch", "Item", "csEvent")
  step("unmenuMissing", removeCommandLineMenuEvent, "csNoSuch", "Item")

  step("select", selectCmdLineText, "csLine")
  printCmdLine("csLine", "some text")
  step("selectText", selectCmdLineText, "csLine")
  step("textAfterSelect", getCmdLine, "csLine")
  step("selectMissing", selectCmdLineText, "csNoSuch")

  step("history", getSaveCommandHistory, "csLine")
  step("historyOff", setSaveCommandHistory, "csLine", false)
  step("historyAfterOff", getSaveCommandHistory, "csLine")
  step("historyOn", setSaveCommandHistory, "csLine", true)
  step("historyAfterOn", getSaveCommandHistory, "csLine")
  step("historyOffMissing", setSaveCommandHistory, "csNoSuch", false)
  step("historyMissing", getSaveCommandHistory, "csNoSuch")

  step("disable", disableCommandLine, "csLine")
  look("disabled", "csLine")
  step("disableAgain", disableCommandLine, "csLine")
  step("enable", enableCommandLine, "csLine")
  look("enabled", "csLine")
  step("disableMissing", disableCommandLine, "csNoSuch")
  step("enableMissing", enableCommandLine, "csNoSuch")
  step("disableMain", disableCommandLine, "main")
  step("enableEmpty", enableCommandLine, "")
  hideWindow("csLine")
  step("enableHidden", enableCommandLine, "csLine")
  look("enabledAfterHide", "csLine")
  step("disableShown", disableCommandLine, "csLine")
  step("showDisabled", showWindow, "csLine")
  look("shownAfterDisable", "csLine")

  -- A command line in a hidden user window stays out of sight when enabled
  assert(openUserWindow("csWindow", false, false, "f") == true, "openUserWindow did not answer true")
  createCommandLine("csWindow", "csWin", 1, 1, 50, 20)
  disableCommandLine("csWin")
  hideWindow("csWindow")
  step("enableInHidden", enableCommandLine, "csWin")
  look("enabledInHidden", "csWin")
  showWindow("csWindow")
  look("windowShown", "csWin")

  -- The name reads as the scroll box while there is one, but these act on the command line
  assert(createScrollBox("csShared", 1, 2, 100, 100))
  createCommandLine("csShared", 5, 6, 40, 20)
  step("disableShared", disableCommandLine, "csShared")
  look("sharedDisabled", "csShared")
  step("menuShared", addCommandLineMenuEvent, "csShared", "Item", "csEvent")
  step("historyShared", setSaveCommandHistory, "csShared", false)
  step("historySharedAfter", getSaveCommandHistory, "csShared")
  deleteScrollBox("csShared")
  look("sharedBoxDeleted", "csShared")
  step("unmenuShared", removeCommandLineMenuEvent, "csShared", "Item")

  -- A text edit is not a command line
  createTextEdit("csEdit", 0, 0, 50, 50)
  step("suggestEdit", addCmdLineSuggestion, "csEdit", "word")
  step("menuEdit", addCommandLineMenuEvent, "csEdit", "Item", "csEvent")
  step("unmenuEdit", removeCommandLineMenuEvent, "csEdit", "Item")
  step("selectEdit", selectCmdLineText, "csEdit")
  step("historyEdit", setSaveCommandHistory, "csEdit", false)
  step("enableEdit", enableCommandLine, "csEdit")
  step("disableEdit", disableCommandLine, "csEdit")
  look("editAfterDisable", "csEdit")

  -- What is set goes with a deleted command line
  addCommandLineMenuEvent("csLine", "Kept", "csEvent")
  setSaveCommandHistory("csLine", false)
  deleteCommandLine("csLine")
  step("suggestDeleted", addCmdLineSuggestion, "csLine", "word")
  step("blacklistDeleted", addCmdLineBlacklist, "csLine", "word")
  step("unmenuDeleted", removeCommandLineMenuEvent, "csLine", "Kept")
  step("selectDeleted", selectCmdLineText, "csLine")
  step("historyDeleted", setSaveCommandHistory, "csLine", true)
  step("getHistoryDeleted", getSaveCommandHistory, "csLine")
  step("enableDeleted", enableCommandLine, "csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("unmenuRecreated", removeCommandLineMenuEvent, "csLine", "Kept")
  step("historyRecreated", getSaveCommandHistory, "csLine")

  -- The history setting a profile save writes is the one a command line of the name is made with
  setSaveCommandHistory("csLine", false)
  step("save", save)
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyAfterSave", getSaveCommandHistory, "csLine")
  setSaveCommandHistory("csLine", true)
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyNotSaved", getSaveCommandHistory, "csLine")
  setSaveCommandHistory("csLine", true)
  step("saveTrue", save)
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyAfterSavedTrue", getSaveCommandHistory, "csLine")
  -- Only a save with no folder or name is the end of session save that writes the setting
  setSaveCommandHistory("csLine", false)
  step("saveElsewhere", save, getMudletHomeDir() .. "/csElsewhere")
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyAfterSaveElsewhere", getSaveCommandHistory, "csLine")
  setSaveCommandHistory("csLine", false)
  deleteCommandLine("csLine")
  step("saveAfterDelete", save)
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyDeletedBeforeSave", getSaveCommandHistory, "csLine")
  -- The command line made last writes last, as the GUI connects them to the save in the order they are made
  openUserWindow("csOrder", false, false, "f")
  step("createOrder", createCommandLine, "csOrder", "csOrder", 0, 0, 50, 20)
  setSaveCommandHistory("csOrder", false)
  step("enableOrder", enableCommandLine, "csOrder")
  deleteMiniConsole("csOrder")
  step("saveOrder", save)
  step("iniOrder", headlessCommandLineIni, "csOrder")
  createCommandLine("csKey_b", 0, 0, 50, 20)
  setSaveCommandHistory("csKey_b", false)
  createCommandLine("csKey/b", 0, 0, 50, 20)
  step("saveKey", save)
  step("iniKey", headlessCommandLineIni, "csKey_b")
  deleteCommandLine("csKey_b")
  deleteCommandLine("csKey/b")
  -- An MXP frame goes at once, its command line with it
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  step("enableFrame", enableCommandLine, "csFrame")
  setSaveCommandHistory("csFrame", false)
  feedTriggers([[<FRAME csFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("frameClosed", windowType, "csFrame")
  step("saveFrame", save)
  step("iniFrame", headlessCommandLineIni, "csFrame")
  -- A closed frame takes a command line deleted from it at once, as does one whose console was deleted
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csFrameLine" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csFrameLine")
  setSaveCommandHistory("csFrameLine", false)
  deleteCommandLine("csFrameLine")
  feedTriggers([[<FRAME csFrameLine ACTION="close">]] .. "\n")
  feedTriggers([[<FRAME Name="csFrameConsole" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csFrameConsole")
  setSaveCommandHistory("csFrameConsole", false)
  deleteMiniConsole("csFrameConsole")
  feedTriggers([[<FRAME csFrameConsole ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("saveFrames", save)
  step("iniFrameLine", headlessCommandLineIni, "csFrameLine")
  step("iniFrameConsole", headlessCommandLineIni, "csFrameConsole")
  -- A frame's console moved out of the frame outlives it, out of the registry, with its command line
  setConfig("specialForceMXPProcessorOn", true)
  openUserWindow("csFrameHome", false, false, "f")
  feedTriggers([[<FRAME Name="csMovedFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csMovedFrame")
  setSaveCommandHistory("csMovedFrame", false)
  step("moveFrameConsole", setWindow, "csFrameHome", "csMovedFrame", 0, 0, true)
  feedTriggers([[<FRAME csMovedFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("movedFrameType", windowType, "csMovedFrame")
  step("movedFrameHistory", getSaveCommandHistory, "csMovedFrame")
  step("movedFrameText", getCmdLine, "csMovedFrame")
  step("movedFrameVisible", windowVisible, "csMovedFrame")
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csHiddenFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csHiddenFrame")
  setWindow("csFrameHome", "csHiddenFrame", 0, 0, true)
  disableCommandLine("csHiddenFrame")
  feedTriggers([[<FRAME csHiddenFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("hiddenFrameLineVisible", windowVisible, "csHiddenFrame")
  step("hiddenFrameLineEnable", enableCommandLine, "csHiddenFrame")
  step("hiddenFrameLineShown", windowVisible, "csHiddenFrame")
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csLateFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  setWindow("csFrameHome", "csLateFrame", 0, 0, true)
  enableCommandLine("csLateFrame")
  feedTriggers([[<FRAME csLateFrame ACTION="close">]] .. "\n")
  step("lateFrameLineVisibleAtOnce", windowVisible, "csLateFrame")
  feedTriggers([[<FRAME Name="csHiddenConsole" Align="right" Width="20%" Height="30%">]] .. "\n")
  setWindow("csFrameHome", "csHiddenConsole", 0, 0, true)
  enableCommandLine("csHiddenConsole")
  hideWindow("csHiddenConsole")
  feedTriggers([[<FRAME csHiddenConsole ACTION="close">]] .. "\n")
  step("hiddenConsoleLineVisibleAtOnce", windowVisible, "csHiddenConsole")
  setConfig("specialForceMXPProcessorOn", false)
  hideWindow("csFrameHome")
  showWindow("csFrameHome")
  step("lateFrameLineVisible", windowVisible, "csLateFrame")
  step("hiddenConsoleLineVisible", windowVisible, "csHiddenConsole")
  -- Moving a frame's console into the main window takes it out of the frame, which hides it
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csMainFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csMainFrame")
  showWindow("csMainFrame")
  step("moveFrameToMain", setWindow, "main", "csMainFrame", 0, 0, false)
  step("frameInMainVisible", windowVisible, "csMainFrame")
  feedTriggers([[<FRAME csMainFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("frameInMainLineVisible", windowVisible, "csMainFrame")
  -- Moving a console's command line out of the console takes it out of the console's layer
  step("moveHiddenFrameLine", setWindow, "main", "csHiddenFrame", 0, 0, true)
  step("hiddenFrameLineMovedVisible", windowVisible, "csHiddenFrame")
  step("moveLateFrameLine", setWindow, "csFrameHome", "csLateFrame", 5, 5, false)
  step("lateFrameLineMovedVisible", windowVisible, "csLateFrame")
  -- An external frame is its console, which goes with it wherever it was moved
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csExternal" EXTERNAL Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csExternal")
  setSaveCommandHistory("csExternal", false)
  step("moveExternal", setWindow, "csFrameHome", "csExternal", 0, 0, true)
  feedTriggers([[<FRAME csExternal ACTION="close">]] .. "\n")
  step("externalType", windowType, "csExternal")
  step("externalHistory", getSaveCommandHistory, "csExternal")
  -- An external frame is not made over a script's console of its name
  createMiniConsole("csClaimed", 0, 0, 50, 50)
  enableCommandLine("csClaimed")
  printCmdLine("csClaimed", "mine")
  feedTriggers([[<FRAME Name="csClaimed" EXTERNAL Width="20%" Height="30%">]] .. "\n")
  feedTriggers([[<FRAME csClaimed ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("claimedType", windowType, "csClaimed")
  step("claimedText", getCmdLine, "csClaimed")
  step("claimedDisable", disableCommandLine, "csClaimed")
  step("claimedTextAfter", getCmdLine, "csClaimed")
  deleteMiniConsole("csClaimed")
  -- A command line a console's own one took the name from keeps its Lua function
  local held = setmetatable({}, {__mode = "k"})
  local action = function() end
  held[action] = true
  createCommandLine("csHeldAction", 0, 0, 50, 20)
  setCmdLineAction("csHeldAction", action)
  action = nil
  createMiniConsole("csHeldAction", 0, 0, 50, 50)
  enableCommandLine("csHeldAction")
  collectgarbage()
  collectgarbage()
  step("displacedAction", function()
    return next(held) and "kept" or "freed"
  end)
  deleteMiniConsole("csHeldAction")
  -- One deleted in an event loop run from a script goes as that loop delivers deferred deletes
  createCommandLine("csOuter", 0, 0, 50, 20)
  createCommandLine("csNested", 0, 30, 50, 20)
  deleteCommandLine("csOuter")
  local nestedFired, nestedOk, nestedError = false, false, nil
  -- Caught here, as a timer's own error goes to the error console and would read as one that never fired
  tempTimer(0, function()
    nestedOk, nestedError = pcall(function()
      setSaveCommandHistory("csNested", false)
      deleteCommandLine("csNested")
    end)
    nestedFired = true
  end)
  -- A loaded machine can fire the timer on a wait's last pass, which returns before delivering the delete
  for _ = 1, 200 do
    if nestedFired then
      break
    end
    pumpEvents(50)
  end
  if not nestedFired then
    error("the timer deleting csNested did not fire")
  end
  if not nestedOk then
    error("the timer deleting csNested raised: " .. tostring(nestedError))
  end
  pumpEvents(50)
  step("saveNested", save)
  step("iniNested", headlessCommandLineIni, "csNested")

  -- A console's own command line answers to the console's name, and is made the first time it is shown
  createMiniConsole("csMini", 0, 0, 100, 100)
  step("disableMini", disableCommandLine, "csMini")
  step("getMini", getCmdLine, "csMini")
  step("suggestMini", addCmdLineSuggestion, "csMini", "word")
  step("enableMini", enableCommandLine, "csMini")
  look("miniEnabled", "csMini")
  step("getMiniEnabled", getCmdLine, "csMini")
  step("printMini", printCmdLine, "csMini", "mini text")
  step("getMiniPrinted", getCmdLine, "csMini")
  step("suggestMiniEnabled", addCmdLineSuggestion, "csMini", "word")
  step("menuMini", addCommandLineMenuEvent, "csMini", "Item", "csEvent")
  step("historyMini", getSaveCommandHistory, "csMini")
  step("disableMiniEnabled", disableCommandLine, "csMini")
  look("miniDisabled", "csMini")
  step("getMiniDisabled", getCmdLine, "csMini")
  step("createOverMini", createCommandLine, "csMini", 0, 0, 10, 10)
  step("deleteMiniLine", deleteCommandLine, "csMini")
  step("getMiniLineDeleted", getCmdLine, "csMini")
  step("deleteMini", deleteMiniConsole, "csMini")
  step("enableMiniDeleted", enableCommandLine, "csMini")
  -- Until deferred deletes run a deleted console's command line still writes its setting on a save
  createMiniConsole("csGone", 0, 0, 100, 100)
  enableCommandLine("csGone")
  setSaveCommandHistory("csGone", false)
  deleteMiniConsole("csGone")
  step("saveGone", save)
  createMiniConsole("csGone", 0, 0, 100, 100)
  step("enableGoneRemade", enableCommandLine, "csGone")
  step("historyGoneRemade", getSaveCommandHistory, "csGone")
  deleteMiniConsole("csGone")
  -- A deleted console's command line is not made again until deferred deletes run
  createMiniConsole("csDying", 0, 0, 100, 100)
  enableCommandLine("csDying")
  deleteCommandLine("csDying")
  step("enableDying", enableCommandLine, "csDying")
  step("getDying", getCmdLine, "csDying")
  step("createOverDying", createCommandLine, "csDying", 5, 6, 50, 20)
  printCmdLine("csDying", "standalone")
  step("enableOverDying", enableCommandLine, "csDying")
  step("getOverDying", getCmdLine, "csDying")
  step("disableOverDying", disableCommandLine, "csDying")
  deleteMiniConsole("csDying")
  look("dyingMiniDeleted", "csDying")
  deleteCommandLine("csDying")
  -- A console made again under the name has none yet
  createMiniConsole("csDying", 0, 0, 100, 100)
  step("enableRemadeDying", enableCommandLine, "csDying")
  step("getRemadeDying", getCmdLine, "csDying")
  deleteMiniConsole("csDying")
  -- Hiding a console's command line it never showed leaves a command line of the same name alone
  createMiniConsole("csTwin", 0, 0, 100, 100)
  createCommandLine("csTwin", 5, 6, 50, 20)
  step("disableTwin", disableCommandLine, "csTwin")
  look("twinDisabled", "csTwin")
  deleteMiniConsole("csTwin")
  look("twinMiniDeleted", "csTwin")
  step("disableTwinLine", disableCommandLine, "csTwin")
  look("twinLineDisabled", "csTwin")
  deleteCommandLine("csTwin")
  -- Showing it takes the name from a command line of the same name
  createMiniConsole("csChat", 0, 0, 100, 100)
  createCommandLine("csChat", 5, 6, 50, 20)
  printCmdLine("csChat", "standalone")
  addCommandLineMenuEvent("csChat", "Item", "csEvent")
  step("enableChat", enableCommandLine, "csChat")
  step("getChat", getCmdLine, "csChat")
  step("unmenuChat", removeCommandLineMenuEvent, "csChat", "Item")
  step("deleteChatLine", deleteCommandLine, "csChat")
  step("getChatLineDeleted", getCmdLine, "csChat")
  deleteMiniConsole("csChat")
  look("chatDeleted", "csChat")
  -- A user window and a buffer are consoles too, a label is not
  step("disableWindow", disableCommandLine, "csWindow")
  step("enableWindow", enableCommandLine, "csWindow")
  step("getWindow", getCmdLine, "csWindow")
  look("windowEnabled", "csWindow")
  createBuffer("csBuffer")
  step("disableBuffer", disableCommandLine, "csBuffer")
  createLabel("csLabel", 0, 0, 10, 10, 1)
  step("disableLabel", disableCommandLine, "csLabel")
  deleteLabel("csLabel")

  deleteCommandLine("csLine")
  deleteCommandLine("csShared")
  deleteTextEdit("csEdit")
  closeUserWindow("csWindow")
  headlessCommandLineSettingsAnswers = table.concat(answers, "\n")

  -- The main command line
  answers = {}
  step("historyNoName", getSaveCommandHistory)
  step("historyMain", getSaveCommandHistory, "main")
  step("setHistoryNoName", setSaveCommandHistory, false)
  step("setHistoryMain", setSaveCommandHistory, "main", false)
  setSaveCommandHistory("main", true)
  step("suggestMain", addCmdLineSuggestion, "main", "word")
  step("suggestNoName", addCmdLineSuggestion, "", "word")
  step("blacklistMain", addCmdLineBlacklist, "main", "word")
  step("selectMain", selectCmdLineText, "main")
  step("menuMain", addCommandLineMenuEvent, "main", "Item", "csEvent")
  step("unmenuMain", removeCommandLineMenuEvent, "main", "Item")
  headlessCommandLineMainAnswers = table.concat(answers, "\n")
end)
headlessCommandLineSettings = ok and "ok" or tostring(err)
)lua"));
            pendingDeleteHeld = !pPending.isNull();
            loop.quit();
        });
        loop.exec();
        QVERIFY2(ran, "The Lua chunk did not run.");
        QVERIFY2(pendingDeleteHeld, "Waiting for a save delivered a deferred delete, which the GUI's waits do not.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineSettings"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("suggest=[0]"),
                                  qsl("suggestAgain=[0]"),
                                  qsl("unsuggest=[0]"),
                                  qsl("unsuggestAbsent=[0]"),
                                  qsl("clearSuggestions=[0]"),
                                  qsl("suggestMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("unsuggestMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("clearSuggestionsMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("suggestEmpty=[0]"),
                                  qsl("suggestBadText=raised:addCmdLineSuggestion: bad argument #2 type (suggestion text as string expected, got table!)"),
                                  qsl("blacklist=[0]"),
                                  qsl("unblacklist=[0]"),
                                  qsl("unblacklistAbsent=[0]"),
                                  qsl("clearBlacklist=[0]"),
                                  qsl("blacklistMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("unblacklistMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("clearBlacklistMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("menu=[1]true"),
                                  qsl("menuAgain=[1]true"),
                                  qsl("menuOther=[1]true"),
                                  qsl("unmenu=[1]true"),
                                  qsl("unmenuAgain=[2]nil:cannot remove 'Item', menu item does not exist"),
                                  qsl("unmenuOther=[1]true"),
                                  qsl("menuMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("unmenuMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("select=[1]true"),
                                  qsl("selectText=[1]true"),
                                  qsl("textAfterSelect=[1]some text"),
                                  qsl("selectMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("history=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyOff=[1]true"),
                                  qsl("historyAfterOff=[2]false:disabled"),
                                  qsl("historyOn=[1]true"),
                                  qsl("historyAfterOn=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyOffMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("historyMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("disable=[1]true"),
                                  qsl("disabled:type=[1]commandline,visible=[1]false"),
                                  qsl("disableAgain=[1]true"),
                                  qsl("enable=[1]true"),
                                  qsl("enabled:type=[1]commandline,visible=[1]true"),
                                  qsl("disableMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("enableMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("disableMain=[2]nil:this function is not permitted on the main command line"),
                                  qsl("enableEmpty=[2]nil:this function is not permitted on the main command line"),
                                  qsl("enableHidden=[1]true"),
                                  qsl("enabledAfterHide:type=[1]commandline,visible=[1]true"),
                                  qsl("disableShown=[1]true"),
                                  qsl("showDisabled=[1]true"),
                                  qsl("shownAfterDisable:type=[1]commandline,visible=[1]true"),
                                  qsl("enableInHidden=[1]true"),
                                  qsl("enabledInHidden:type=[1]commandline,visible=[1]false"),
                                  qsl("windowShown:type=[1]commandline,visible=[1]true"),
                                  qsl("disableShared=[1]true"),
                                  qsl("sharedDisabled:type=[1]scrollbox,visible=[1]true"),
                                  qsl("menuShared=[1]true"),
                                  qsl("historyShared=[1]true"),
                                  qsl("historySharedAfter=[2]false:disabled"),
                                  qsl("sharedBoxDeleted:type=[1]commandline,visible=[1]false"),
                                  qsl("unmenuShared=[1]true"),
                                  qsl("suggestEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("menuEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("unmenuEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("selectEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("historyEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("enableEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("disableEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("editAfterDisable:type=[1]textedit,visible=[1]true"),
                                  qsl("suggestDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("blacklistDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("unmenuDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("selectDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("historyDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("getHistoryDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("enableDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("unmenuRecreated=[2]nil:cannot remove 'Kept', menu item does not exist"),
                                  qsl("historyRecreated=[2]true:enabled (500 lines will be saved)"),
                                  qsl("save=[1]true"),
                                  qsl("historyAfterSave=[2]false:disabled"),
                                  qsl("historyNotSaved=[2]false:disabled"),
                                  qsl("saveTrue=[1]true"),
                                  qsl("historyAfterSavedTrue=[2]true:enabled (500 lines will be saved)"),
                                  qsl("saveElsewhere=[1]true"),
                                  qsl("historyAfterSaveElsewhere=[2]true:enabled (500 lines will be saved)"),
                                  qsl("saveAfterDelete=[1]true"),
                                  qsl("historyDeletedBeforeSave=[2]false:disabled"),
                                  qsl("createOrder=[1]true"),
                                  qsl("enableOrder=[1]true"),
                                  qsl("saveOrder=[1]true"),
                                  qsl("iniOrder=[1]true"),
                                  qsl("saveKey=[1]true"),
                                  qsl("iniKey=[1]true"),
                                  qsl("enableFrame=[1]true"),
                                  qsl("frameClosed=[2]nil:'csFrame' is not a known label, any type of console, command line, text edit, nor scroll box"),
                                  qsl("saveFrame=[1]true"),
                                  qsl("iniFrame=[1]true"),
                                  qsl("saveFrames=[1]true"),
                                  qsl("iniFrameLine=[1]true"),
                                  qsl("iniFrameConsole=[1]true"),
                                  qsl("moveFrameConsole=[1]true"),
                                  qsl("movedFrameType=[1]commandline"),
                                  qsl("movedFrameHistory=[2]false:disabled"),
                                  qsl("movedFrameText=[1]"),
                                  qsl("movedFrameVisible=[1]true"),
                                  qsl("hiddenFrameLineVisible=[1]false"),
                                  qsl("hiddenFrameLineEnable=[1]true"),
                                  qsl("hiddenFrameLineShown=[1]false"),
                                  qsl("lateFrameLineVisibleAtOnce=[1]true"),
                                  qsl("hiddenConsoleLineVisibleAtOnce=[1]false"),
                                  qsl("lateFrameLineVisible=[1]true"),
                                  qsl("hiddenConsoleLineVisible=[1]false"),
                                  qsl("moveFrameToMain=[1]true"),
                                  qsl("frameInMainVisible=[1]false"),
                                  qsl("frameInMainLineVisible=[1]false"),
                                  qsl("moveHiddenFrameLine=[1]true"),
                                  qsl("hiddenFrameLineMovedVisible=[1]true"),
                                  qsl("moveLateFrameLine=[1]true"),
                                  qsl("lateFrameLineMovedVisible=[1]false"),
                                  qsl("moveExternal=[1]true"),
                                  qsl("externalType=[2]nil:'csExternal' is not a known label, any type of console, command line, text edit, nor scroll box"),
                                  qsl("externalHistory=[2]nil:command line \"csExternal\" not found"),
                                  qsl("claimedType=[1]miniconsole"),
                                  qsl("claimedText=[1]mine"),
                                  qsl("claimedDisable=[1]true"),
                                  qsl("claimedTextAfter=[1]mine"),
                                  qsl("displacedAction=[1]kept"),
                                  qsl("saveNested=[1]true"),
                                  qsl("iniNested=[1]true"),
                                  qsl("disableMini=[1]true"),
                                  qsl("getMini=[2]nil:command line \"csMini\" not found"),
                                  qsl("suggestMini=[2]nil:command line \"csMini\" not found"),
                                  qsl("enableMini=[1]true"),
                                  qsl("miniEnabled:type=[1]miniconsole,visible=[1]true"),
                                  qsl("getMiniEnabled=[1]"),
                                  qsl("printMini=[0]"),
                                  qsl("getMiniPrinted=[1]mini text"),
                                  qsl("suggestMiniEnabled=[0]"),
                                  qsl("menuMini=[1]true"),
                                  qsl("historyMini=[2]true:enabled (500 lines will be saved)"),
                                  qsl("disableMiniEnabled=[1]true"),
                                  qsl("miniDisabled:type=[1]miniconsole,visible=[1]true"),
                                  qsl("getMiniDisabled=[1]mini text"),
                                  qsl("createOverMini=[2]false:couldn't create commandLine"),
                                  qsl("deleteMiniLine=[1]true"),
                                  qsl("getMiniLineDeleted=[2]nil:command line \"csMini\" not found"),
                                  qsl("deleteMini=[1]true"),
                                  qsl("enableMiniDeleted=[2]nil:command line \"csMini\" not found"),
                                  qsl("saveGone=[1]true"),
                                  qsl("enableGoneRemade=[1]true"),
                                  qsl("historyGoneRemade=[2]false:disabled"),
                                  qsl("enableDying=[1]true"),
                                  qsl("getDying=[2]nil:command line \"csDying\" not found"),
                                  qsl("createOverDying=[1]true"),
                                  qsl("enableOverDying=[1]true"),
                                  qsl("getOverDying=[1]standalone"),
                                  qsl("disableOverDying=[1]true"),
                                  qsl("dyingMiniDeleted:type=[1]commandline,visible=[1]true"),
                                  qsl("enableRemadeDying=[1]true"),
                                  qsl("getRemadeDying=[1]"),
                                  qsl("disableTwin=[1]true"),
                                  qsl("twinDisabled:type=[1]miniconsole,visible=[1]true"),
                                  qsl("twinMiniDeleted:type=[1]commandline,visible=[1]true"),
                                  qsl("disableTwinLine=[1]true"),
                                  qsl("twinLineDisabled:type=[1]commandline,visible=[1]false"),
                                  qsl("enableChat=[1]true"),
                                  qsl("getChat=[1]"),
                                  qsl("unmenuChat=[2]nil:cannot remove 'Item', menu item does not exist"),
                                  qsl("deleteChatLine=[1]true"),
                                  qsl("getChatLineDeleted=[2]nil:command line \"csChat\" not found"),
                                  qsl("chatDeleted:type=[2]nil:'csChat' is not a known label, any type of console, command line, text edit, nor scroll box,visible=[2]nil:window \"csChat\" not found"),
                                  qsl("disableWindow=[1]true"),
                                  qsl("enableWindow=[1]true"),
                                  qsl("getWindow=[1]"),
                                  qsl("windowEnabled:type=[1]userwindow,visible=[1]true"),
                                  qsl("disableBuffer=[1]true"),
                                  qsl("disableLabel=[2]nil:command line \"csLabel\" not found")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineSettingsAnswers"), answers.join(QLatin1Char('\n')));
        const QStringList mainAnswers{qsl("historyNoName=[2]true:enabled (500 lines will be saved)"),
                                      qsl("historyMain=[2]true:enabled (500 lines will be saved)"),
                                      qsl("setHistoryNoName=[1]true"),
                                      qsl("setHistoryMain=[1]true"),
                                      qsl("suggestMain=[0]"),
                                      qsl("suggestNoName=[0]"),
                                      qsl("blacklistMain=[0]"),
                                      qsl("selectMain=[1]true"),
                                      qsl("menuMain=[1]true"),
                                      qsl("unmenuMain=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineMainAnswers"), mainAnswers.join(QLatin1Char('\n')));

        // A command line removed before deferred deletes run still writes its setting, as its TCommandLine
        // does until then, and one removed after does not
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createCommandLine("csLine", 10, 20, 200, 30)
setSaveCommandHistory("csLine", true)
headlessCommandLineSave()
setSaveCommandHistory("csLine", false)
deleteCommandLine("csLine")
createCommandLine("csSavedLater", 0, 0, 50, 20)
setSaveCommandHistory("csSavedLater", false)
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the command line did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineSave()
createCommandLine("csLine", 10, 20, 200, 30)
headlessCommandLineDeferred = tostring(getSaveCommandHistory("csLine"))
deleteCommandLine("csLine")
)lua"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineDeferred"), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csSavedLater")), qsl("false"));

        // A console's own command line goes with the console, even one moved out of a closed frame, and one in a
        // deleted user window writes its setting until deferred deletes run
        QTRY_VERIFY(!host->currentlySavingProfile());
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createMiniConsole("csGone", 0, 0, 100, 100)
enableCommandLine("csGone")
deleteMiniConsole("csGone")
createMiniConsole("csRevived", 0, 0, 100, 100)
enableCommandLine("csRevived")
deleteCommandLine("csRevived")
openUserWindow("csGoneWindow", false, false, "f")
createCommandLine("csGoneWindow", "csInGoneWindow", 0, 0, 50, 20)
setSaveCommandHistory("csInGoneWindow", false)
deleteMiniConsole("csGoneWindow")
setConfig("specialForceMXPProcessorOn", true)
openUserWindow("csOrphanHome", false, false, "f")
feedTriggers([[<FRAME Name="csOrphanA" Align="right" Width="20%" Height="30%">]] .. "\n")
enableCommandLine("csOrphanA")
setWindow("csOrphanHome", "csOrphanA", 0, 0, true)
feedTriggers([[<FRAME Name="csOrphanB" Align="right" Width="20%" Height="30%">]] .. "\n")
setWindow("csOrphanHome", "csOrphanB", 0, 0, true)
enableCommandLine("csOrphanB")
feedTriggers([[<FRAME csOrphanA ACTION="close">]] .. "\n")
feedTriggers([[<FRAME csOrphanB ACTION="close">]] .. "\n")
setConfig("specialForceMXPProcessorOn", false)
deleteMiniConsole("csOrphanHome")
headlessCommandLineSave()
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the consoles did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineGone = tostring(getCmdLine("csGone")) .. ":" .. tostring(windowType("csGone"))
headlessCommandLineOrphans = tostring(windowType("csOrphanA")) .. ":" .. tostring(windowType("csOrphanB"))
enableCommandLine("csRevived")
headlessCommandLineRevived = tostring(getCmdLine("csRevived"))
deleteMiniConsole("csRevived")
createCommandLine("csInGoneWindow", 0, 0, 50, 20)
headlessCommandLineRemade = tostring(getSaveCommandHistory("csInGoneWindow"))
deleteCommandLine("csInGoneWindow")
)lua"));
        QVERIFY2(ran, "The Lua chunk after the consoles went did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineGone"), qsl("nil:nil"));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineOrphans"), qsl("nil:nil"));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineRevived"), QString());
        QCOMPARE(luaGlobalString(host, "headlessCommandLineRemade"), qsl("false"));

        // A command line a console's own one took the name from goes on writing its setting, and a text edit of
        // the name reads as itself again once the console goes
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createMiniConsole("csDisplaced", 0, 0, 100, 100)
createCommandLine("csDisplaced", 5, 5, 50, 20)
setSaveCommandHistory("csDisplaced", false)
enableCommandLine("csDisplaced")
deleteMiniConsole("csDisplaced")
createTextEdit("csShadowed", 1, 2, 50, 50)
createMiniConsole("csShadowed", 0, 0, 100, 100)
enableCommandLine("csShadowed")
disableCommandLine("csShadowed")
openUserWindow("csShadowWindow", false, false, "f")
hideWindow("csShadowWindow")
showWindow("csShadowWindow")
deleteMiniConsole("csShadowed")
)lua"));
        QVERIFY2(ran, "The Lua chunk displacing the command line did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineSave()
headlessCommandLineShadowed = table.concat({tostring(windowType("csShadowed")), tostring(windowVisible("csShadowed")), table.concat({getWindowGeometry("csShadowed")}, ",")}, ":")
deleteTextEdit("csShadowed")
)lua"));
        QVERIFY2(ran, "The Lua chunk after the console went did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csDisplaced")), qsl("false"));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineShadowed"), qsl("textedit:true:1,2,50,50"));

        // Once deferred deletes run, a command line in a deleted window writes nothing more, wherever it was
        // moved, and so does one a console's own one took the name from
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createScrollBox("csBoxed", 0, 0, 100, 100)
createMiniConsole("csBoxed", "csBoxedMini", 0, 0, 50, 50)
enableCommandLine("csBoxedMini")
setSaveCommandHistory("csBoxedMini", false)
createCommandLine("csBoxed", "csBoxedLine", 0, 60, 50, 20)
setSaveCommandHistory("csBoxedLine", false)
deleteScrollBox("csBoxed")
createScrollBox("csMoved", 0, 0, 100, 100)
createCommandLine("csMoved", "csMovedLine", 0, 0, 50, 20)
setSaveCommandHistory("csMovedLine", false)
createMiniConsole("csMovedLine", 0, 0, 50, 50)
enableCommandLine("csMovedLine")
openUserWindow("csMovedWindow", false, false, "f")
setWindow("csMovedWindow", "csMoved", 0, 0, true)
deleteMiniConsole("csMovedLine")
deleteMiniConsole("csMovedWindow")
createMiniConsole("csOwnGone", 0, 0, 50, 50)
enableCommandLine("csOwnGone")
setSaveCommandHistory("csOwnGone", false)
deleteMiniConsole("csOwnGone")
setSaveCommandHistory("csSavedLater", true)
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the windows did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessCommandLineSave()"));
        QVERIFY2(ran, "The save after the windows went did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        for (const QString& name : {qsl("csBoxedMini"), qsl("csBoxedLine"), qsl("csMovedLine"), qsl("csOwnGone"), qsl("csSavedLater")}) {
            QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/%1").arg(name)), qsl("true"));
        }

        // A frame nested in another goes with it as deferred deletes run, with its command line
        TMxpFrameFrontend& frames = host->consoleFrontend()->mxpFrames();
        QVERIFY(frames.createExternalFrame(qsl("csHostFrame"), qsl("Host"), QSize(200, 200), true));
        frames.createInternalFrame(qsl("csNestedFrame"), qsl("csHostFrame"), qsl("Nested"), QRect(0, 0, 100, 100), false, true);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineNested = tostring(windowType("csNestedFrame"))
enableCommandLine("csNestedFrame")
setSaveCommandHistory("csNestedFrame", false)
deleteMiniConsole("csHostFrame")
setSaveCommandHistory("csSavedLater", false)
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the host frame did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!host->currentlySavingProfile());
        ran = host->getLuaInterpreter()->compileAndExecuteScript(
                qsl(R"lua(headlessCommandLineNested = headlessCommandLineNested .. ":" .. tostring(windowType("csNestedFrame")) headlessCommandLineSave())lua"));
        QVERIFY2(ran, "The save after the host frame went did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(luaGlobalString(host, "headlessCommandLineNested"), qsl("miniconsole:nil"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csNestedFrame")), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csSavedLater")), qsl("false"));
        // An external frame's widget is its console, which a script's deletion leaves until deferred deletes run
        QVERIFY(frames.createExternalFrame(qsl("csDeletedFrame"), qsl("Deleted"), QSize(200, 200), true));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(deleteMiniConsole("csDeletedFrame"))lua")));
        QVERIFY2(frames.hasFrameWidget(qsl("csDeletedFrame")), "A deleted external frame's console went before deferred deletes ran.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(!frames.hasFrameWidget(qsl("csDeletedFrame")), "A deleted external frame's console outlived deferred deletes.");
        // Closing a frame deletes a frame nested in it at once, with its command line
        frames.createInternalFrame(qsl("csOuterFrame"), QString(), qsl("Outer"), QRect(0, 0, 200, 200), false, true);
        frames.createInternalFrame(qsl("csInnerFrame"), qsl("csOuterFrame"), qsl("Inner"), QRect(0, 0, 100, 100), false, true);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
enableCommandLine("csInnerFrame")
setSaveCommandHistory("csInnerFrame", false)
setSaveCommandHistory("csSavedLater", true)
)lua")));
        frames.destroyFrame(qsl("csOuterFrame"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineInner = tostring(windowType("csInnerFrame")) .. ":" .. tostring(getCmdLine("csInnerFrame"))
headlessCommandLineSave()
)lua")));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(luaGlobalString(host, "headlessCommandLineInner"), qsl("nil:nil"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csInnerFrame")), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csSavedLater")), qsl("true"));
        // Nothing is built in a frame that has no widget
        frames.createInternalFrame(qsl("csHostlessFrame"), qsl("csDeletedFrame"), qsl("Hostless"), QRect(0, 0, 100, 100), false, true);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessCommandLineHostless = tostring(windowType("csHostlessFrame")))lua")));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineHostless"), qsl("nil"));

        // A command line named "main" writes the main command line's key with the setting it was made with, as
        // in the GUI
        host->writeProfileIniData(qsl("CommandLines/SaveHistory/main"), qsl("false"));
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessCommandLineMainMade = tostring(createCommandLine("main", 0, 0, 10, 10)))lua"));
        QVERIFY2(ran, "The Lua chunk making a command line named main did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineMainMade"), qsl("true"));
        host->writeProfileIniData(qsl("CommandLines/SaveHistory/main"), qsl("true"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessCommandLineSave()")), "The save with a command line named main failed.");
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/main")), qsl("false"));

        // TMainConsole::resetMainConsole() deletes the command lines and scroll boxes, but not a command line
        // displaced in the main window, which goes on writing its setting
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createCommandLine("csResetLine", 0, 0, 50, 20)
setSaveCommandHistory("csResetLine", false)
createScrollBox("csResetBox", 0, 0, 100, 100)
createCommandLine("csResetBox", "csResetBoxed", 0, 0, 50, 20)
setSaveCommandHistory("csResetBoxed", false)
createMiniConsole("csResetBoxed", 0, 0, 50, 50)
enableCommandLine("csResetBoxed")
deleteMiniConsole("csResetBoxed")
createMiniConsole("csReset", 0, 0, 100, 100)
createCommandLine("csReset", 5, 5, 50, 20)
setSaveCommandHistory("csReset", false)
enableCommandLine("csReset")
deleteMiniConsole("csReset")
)lua"));
        QVERIFY2(ran, "The Lua chunk displacing a command line before the reset did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY2(host->resetProfile_phase1(), "The profile reset did not start.");
        QTRY_VERIFY(!host->profileResetInProgress());
        QTRY_VERIFY(!host->currentlySavingProfile());
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(saveProfile())"));
        QVERIFY2(ran, "The save after the reset failed.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csReset")), qsl("false"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csResetLine")), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csResetBoxed")), qsl("true"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the command lines created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessCommandLineStyleSheetContract()
    {
        const QString hostname = qsl("Test Headless Host Command Line Style");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCmdLineStyle = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. "=" .. described(pcall(getWindowGeometry, name))
  end
  local profileId = (getProfileName():gsub(" ", "\\ "))
  local function line(key, sheet, w, h)
    local name = "cs" .. key
    createCommandLine(name, 0, 0, w or 10, h or 10)
    local parts = {described(pcall(setCmdLineStyleSheet, name, sheet)), described(pcall(getCmdLineStyleSheet, name)), described(pcall(getWindowGeometry, name))}
    resizeWindow(name, 5, 500)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    resizeWindow(name, 500, 5)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    answers[#answers + 1] = key .. "=" .. table.concat(parts, " ")
  end
  createCommandLine("csFresh", 0, 0, 10, 10)
  step("fresh.sheet", getCmdLineStyleSheet, "csFresh")
  line("min", "min-width: 50px; min-height: 30px")
  line("max", "max-width: 20px; max-height: 8px", 100, 100)
  line("padded", "min-width: 50px; padding: 2px; border: 3px solid", 1, 1)
  line("width", "width: 100px; min-width: 50px", 1, 1)
  line("rule", "QPlainTextEdit { min-width: 60px }", 1, 1)
  line("ownRule", "TCommandLine { min-height: 60px }", 1, 1)
  line("textBoxRule", "TTextBox { min-height: 60px }", 1, 1)
  line("scrollRule", "QAbstractScrollArea { min-height: 61px }", 1, 1)
  line("frameRule", "QFrame { min-width: 62px }", 1, 1)
  line("labelRule", "QLabel { min-width: 60px }", 1, 1)
  line("native", "border-style: native; min-width: 50px", 1, 1)
  line("nativeWidth", "border: 4px native; min-width: 50px", 1, 1)
  line("solid", "border: 2px solid; min-width: 50px", 1, 1)
  line("none", "border: none; min-width: 50px", 1, 1)
  line("radius", "border-radius: 2px; min-width: 50px", 1, 1)
  line("em", "font-size: 20px; min-width: 2em", 1, 1)
  line("editable", "QPlainTextEdit:editable { min-width: 60px }", 1, 1)
  line("readOnly", "QPlainTextEdit:read-only { min-width: 60px }", 1, 1)
  line("notReadOnly", "QPlainTextEdit:!read-only { min-width: 60px }", 1, 1)
  line("active", "QPlainTextEdit:active { min-width: 60px }", 1, 1)
  line("inactive", "QPlainTextEdit:!active { padding: 2px; min-width: 50px }", 1, 1)
  line("focus", "QPlainTextEdit:focus { min-width: 60px }", 1, 1)
  line("named", "#commandLine_" .. profileId .. "_csnamed { min-width: 60px }", 1, 1)
  line("otherNamed", "#commandLine_" .. profileId .. "_csother { min-width: 60px }", 1, 1)
  line("textEditNamed", "#textEdit_" .. profileId .. "_cstextEditNamed { min-width: 60px }", 1, 1)
  line("star", "* { min-width: 60px }", 1, 1)
  line("background", "QPlainTextEdit { background-color: red; }", 1, 1)
  line("minOverMax", "min-width: 50px; max-width: 20px; min-height: 9px; max-height: 40px", 30, 30)
  -- Clearing a sheet keeps the size and lets it go
  line("cleared", "min-width: 50px; max-height: 8px", 1, 100)
  step("cleared.clear", setCmdLineStyleSheet, "cscleared", "")
  step("cleared.sheet", getCmdLineStyleSheet, "cscleared")
  look("cleared.after", "cscleared")
  resizeWindow("cscleared", 5, 500)
  look("cleared.free", "cscleared")
  -- Missing, deleted and odd names
  step("missing", setCmdLineStyleSheet, "csMissing", "min-width: 50px")
  step("missing.get", getCmdLineStyleSheet, "csMissing")
  createCommandLine("csGone", 0, 0, 10, 10)
  deleteCommandLine("csGone")
  step("gone", setCmdLineStyleSheet, "csGone", "min-width: 50px")
  step("gone.get", getCmdLineStyleSheet, "csGone")
  step("number", setCmdLineStyleSheet, "csFresh", 5)
  step("number.get", getCmdLineStyleSheet, "csFresh")
  step("noSheet", setCmdLineStyleSheet, "csFresh", nil)
  createLabel("csLabel", 0, 0, 10, 10, 1)
  step("labelName", setCmdLineStyleSheet, "csLabel", "x")
  deleteLabel("csLabel")
  -- In a user window and a scroll box, and moved between them
  openUserWindow("csWindow", false, false, "f")
  createCommandLine("csWindow", "csInWindow", 0, 0, 1, 1)
  step("inWindow", setCmdLineStyleSheet, "csInWindow", "min-width: 40px")
  look("inWindow", "csInWindow")
  createScrollBox("csBox", 0, 0, 100, 100)
  createCommandLine("csBox", "csInBox", 0, 0, 1, 1)
  step("inBox", setCmdLineStyleSheet, "csInBox", "min-width: 41px")
  look("inBox", "csInBox")
  step("inBox.moveBox", setWindow, "csWindow", "csBox", 0, 0, true)
  resizeWindow("csInBox", 5, 5)
  look("inBox.box", "csInBox")
  step("inBox.moveOut", setWindow, "main", "csInBox", 3, 3, true)
  resizeWindow("csInBox", 5, 5)
  look("inBox.out", "csInBox")
  step("inBox.get", getCmdLineStyleSheet, "csInBox")
  -- Hidden
  createCommandLine("csHidden", 0, 0, 1, 1)
  hideWindow("csHidden")
  step("hidden", setCmdLineStyleSheet, "csHidden", "min-width: 42px")
  look("hidden", "csHidden")
  -- Sharing a name with a scroll box or a text edit
  createScrollBox("csSharedBox", 0, 0, 30, 30)
  createCommandLine("csSharedBox", 0, 0, 1, 1)
  step("sharedBox", setCmdLineStyleSheet, "csSharedBox", "min-width: 50px")
  step("sharedBox.get", getCmdLineStyleSheet, "csSharedBox")
  look("sharedBox", "csSharedBox")
  deleteScrollBox("csSharedBox")
  look("sharedBox.alone", "csSharedBox")
  resizeWindow("csSharedBox", 5, 5)
  look("sharedBox.resized", "csSharedBox")
  createTextEdit("csSharedEdit", 0, 0, 1, 1)
  createCommandLine("csSharedEdit", 0, 0, 2, 2)
  step("sharedEdit", setCmdLineStyleSheet, "csSharedEdit", "min-width: 50px")
  look("sharedEdit", "csSharedEdit")
  deleteCommandLine("csSharedEdit")
  look("sharedEdit.edit", "csSharedEdit")
  createCommandLine("csSharedEdit2", 0, 0, 2, 2)
  createTextEdit("csSharedEdit2", 0, 0, 1, 1)
  setTextEditStyleSheet("csSharedEdit2", "min-width: 70px")
  step("sharedEdit2", setCmdLineStyleSheet, "csSharedEdit2", "min-height: 30px")
  look("sharedEdit2", "csSharedEdit2")
  -- A console's own command line
  createMiniConsole("csMini", 0, 0, 100, 100)
  step("mini.before", setCmdLineStyleSheet, "csMini", "min-height: 50px")
  enableCommandLine("csMini")
  step("mini.sheet", getCmdLineStyleSheet, "csMini")
  step("mini.set", setCmdLineStyleSheet, "csMini", "min-height: 300px; min-width: 300px")
  step("mini.get", getCmdLineStyleSheet, "csMini")
  disableCommandLine("csMini")
  step("mini.disabled", setCmdLineStyleSheet, "csMini", "min-height: 30px")
  step("mini.disabledGet", getCmdLineStyleSheet, "csMini")
  step("mini.reenabled", enableCommandLine, "csMini")
  step("mini.reenabledGet", getCmdLineStyleSheet, "csMini")
  createCommandLine("csDisplaced", 0, 0, 1, 1)
  setCmdLineStyleSheet("csDisplaced", "min-width: 50px")
  createMiniConsole("csDisplaced", 0, 0, 100, 100)
  enableCommandLine("csDisplaced")
  step("displaced.get", getCmdLineStyleSheet, "csDisplaced")
  openUserWindow("csOwnWindow", false, false, "f")
  enableCommandLine("csOwnWindow")
  step("window.set", setCmdLineStyleSheet, "csOwnWindow", "min-height: 30px")
  step("window.get", getCmdLineStyleSheet, "csOwnWindow")
  closeUserWindow("csOwnWindow")
  createBuffer("csBuffer")
  enableCommandLine("csBuffer")
  step("buffer.set", setCmdLineStyleSheet, "csBuffer", "color: red")
  step("buffer.get", getCmdLineStyleSheet, "csBuffer")
  for key in ("Fresh min max padded width rule ownRule textBoxRule scrollRule frameRule labelRule native nativeWidth solid none radius em editable readOnly notReadOnly active inactive focus named otherNamed textEditNamed star background minOverMax cleared InWindow InBox Hidden SharedBox SharedEdit2"):gmatch("%S+") do
    deleteCommandLine("cs" .. key)
  end
  deleteTextEdit("csSharedEdit")
  deleteTextEdit("csSharedEdit2")
  deleteScrollBox("csBox")
  deleteMiniConsole("csMini")
  deleteMiniConsole("csDisplaced")
  closeUserWindow("csWindow")
  -- A frame console's own command line outlives a frame it was moved out of, and shows its own geometry then
  setConfig("specialForceMXPProcessorOn", true)
  openUserWindow("csFrameWindow", false, false, "f")
  -- Where its console's layout puts it is not modelled, so one a minimum does not hold is only read once moved
  local frames = {
    {"csFrame", "csFrameWindow", "min-height: 2000px; min-width: 2000px"},
    {"csFrameMain", "main", "min-height: 2000px; min-width: 2000px"},
    {"csFrameMax", "csFrameWindow", "max-height: 20px; max-width: 50px"},
  }
  for _, frame in ipairs(frames) do
    local key, window, sheet = frame[1], frame[2], frame[3]
    local minimum = sheet:find("min", 1, true)
    feedTriggers([[<FRAME Name="]] .. key .. [[" Left="0" Top="0" Width="20%" Height="30%">]] .. "\n")
    enableCommandLine(key)
    step(key .. ".sheet", setCmdLineStyleSheet, key, sheet)
    step(key .. ".move", setWindow, window, key, 0, 0, true)
    feedTriggers([[<FRAME ]] .. key .. [[ ACTION="close">]] .. "\n")
    step(key .. ".type", windowType, key)
    if minimum then
      look(key, key)
    end
    resizeWindow(key, 5, 5)
    if minimum then
      look(key .. ".resized", key)
    end
    resizeWindow(key, 500, 500)
    step(key .. ".moveOut", setWindow, "main", key, 1, 1, true)
    look(key .. ".out", key)
    resizeWindow(key, 5, 5)
    look(key .. ".outResized", key)
    resizeWindow(key, 500, 500)
    look(key .. ".outResizedLarge", key)
    step(key .. ".get", getCmdLineStyleSheet, key)
    deleteCommandLine(key)
  end
  closeUserWindow("csFrameWindow")
  setConfig("specialForceMXPProcessorOn", false)
  headlessCmdLineStyleAnswers = table.concat(answers, "\n")
  local mainAnswers = {}
  local function mainStep(label, f, ...)
    mainAnswers[#mainAnswers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  mainStep("main", setCmdLineStyleSheet, "main", "min-height: 5px")
  mainStep("mainGet", getCmdLineStyleSheet, "main")
  mainStep("noName", setCmdLineStyleSheet, "min-height: 5px")
  mainStep("empty", setCmdLineStyleSheet, "", "min-height: 5px")
  mainStep("nilGet", getCmdLineStyleSheet)
  headlessCmdLineStyleMainAnswers = table.concat(mainAnswers, "\n")
end)
headlessCmdLineStyle = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCmdLineStyle"), qsl("ok"));
        // QCss measures a command line's em in the application font at the sheet's size
        QFont sheetFont;
        sheetFont.setPixelSize(20);
        const int twoEm = 2 * QFontMetrics(sheetFont).height();
        // As the chunk escapes the profile name in a selector
        const QString profileId = QString(hostname).replace(QLatin1Char(' '), qsl("\\ "));
        const QStringList answers{qsl("fresh.sheet=[1]QPlainTextEdit{background-color: rgb(0, 0, 0);}"),
                                  qsl("min=[1]true [1]min-width: 50px; min-height: 30px [4]0:0:52:32 [4]0:0:52:500 [4]0:0:500:32"),
                                  qsl("max=[1]true [1]max-width: 20px; max-height: 8px [4]0:0:22:10 [4]0:0:5:10 [4]0:0:22:5"),
                                  qsl("padded=[1]true [1]min-width: 50px; padding: 2px; border: 3px solid [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                                  qsl("width=[1]true [1]width: 100px; min-width: 50px [4]0:0:102:1 [4]0:0:102:500 [4]0:0:500:5"),
                                  qsl("rule=[1]true [1]QPlainTextEdit { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("ownRule=[1]true [1]TCommandLine { min-height: 60px } [4]0:0:1:62 [4]0:0:5:500 [4]0:0:500:62"),
                                  qsl("textBoxRule=[1]true [1]TTextBox { min-height: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("scrollRule=[1]true [1]QAbstractScrollArea { min-height: 61px } [4]0:0:1:63 [4]0:0:5:500 [4]0:0:500:63"),
                                  qsl("frameRule=[1]true [1]QFrame { min-width: 62px } [4]0:0:64:1 [4]0:0:64:500 [4]0:0:500:5"),
                                  qsl("labelRule=[1]true [1]QLabel { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("native=[1]true [1]border-style: native; min-width: 50px [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                                  qsl("nativeWidth=[1]true [1]border: 4px native; min-width: 50px [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                                  qsl("solid=[1]true [1]border: 2px solid; min-width: 50px [4]0:0:54:1 [4]0:0:54:500 [4]0:0:500:5"),
                                  qsl("none=[1]true [1]border: none; min-width: 50px [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                                  qsl("radius=[1]true [1]border-radius: 2px; min-width: 50px [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                                  qsl("em=[1]true [1]font-size: 20px; min-width: 2em [4]0:0:%1:1 [4]0:0:%1:500 [4]0:0:500:5").arg(twoEm + 2),
                                  qsl("editable=[1]true [1]QPlainTextEdit:editable { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("readOnly=[1]true [1]QPlainTextEdit:read-only { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("notReadOnly=[1]true [1]QPlainTextEdit:!read-only { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("active=[1]true [1]QPlainTextEdit:active { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("inactive=[1]true [1]QPlainTextEdit:!active { padding: 2px; min-width: 50px } [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                                  qsl("focus=[1]true [1]QPlainTextEdit:focus { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("named=[1]true [1]#commandLine_%1_csnamed { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5").arg(profileId),
                                  qsl("otherNamed=[1]true [1]#commandLine_%1_csother { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5").arg(profileId),
                                  qsl("textEditNamed=[1]true [1]#textEdit_%1_cstextEditNamed { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5").arg(profileId),
                                  qsl("star=[1]true [1]* { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("background=[1]true [1]QPlainTextEdit { background-color: red; } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("minOverMax=[1]true [1]min-width: 50px; max-width: 20px; min-height: 9px; max-height: 40px [4]0:0:52:30 [4]0:0:52:42 [4]0:0:52:11"),
                                  qsl("cleared=[1]true [1]min-width: 50px; max-height: 8px [4]0:0:52:10 [4]0:0:52:10 [4]0:0:500:5"),
                                  qsl("cleared.clear=[1]true"),
                                  qsl("cleared.sheet=[1]"),
                                  qsl("cleared.after=[4]0:0:500:5"),
                                  qsl("cleared.free=[4]0:0:5:500"),
                                  qsl("missing=[2]nil:command-line name 'csMissing' not found"),
                                  qsl("missing.get=[2]nil:command-line name 'csMissing' not found"),
                                  qsl("gone=[2]nil:command-line name 'csGone' not found"),
                                  qsl("gone.get=[2]nil:command-line name 'csGone' not found"),
                                  qsl("number=[1]true"),
                                  qsl("number.get=[1]5"),
                                  qsl("noSheet=raised:setCmdLineStyleSheet: bad argument #2 type (StyleSheet as string expected, got nil!)"),
                                  qsl("labelName=[2]nil:command-line name 'csLabel' not found"),
                                  qsl("inWindow=[1]true"),
                                  qsl("inWindow=[4]0:0:42:1"),
                                  qsl("inBox=[1]true"),
                                  qsl("inBox=[4]0:0:43:1"),
                                  qsl("inBox.moveBox=[1]true"),
                                  qsl("inBox.box=[4]0:0:43:5"),
                                  qsl("inBox.moveOut=[1]true"),
                                  qsl("inBox.out=[4]3:3:43:5"),
                                  qsl("inBox.get=[1]min-width: 41px"),
                                  qsl("hidden=[1]true"),
                                  qsl("hidden=[4]0:0:44:1"),
                                  qsl("sharedBox=[1]true"),
                                  qsl("sharedBox.get=[1]min-width: 50px"),
                                  qsl("sharedBox=[4]0:0:30:30"),
                                  qsl("sharedBox.alone=[4]0:0:52:1"),
                                  qsl("sharedBox.resized=[4]0:0:52:5"),
                                  qsl("sharedEdit=[1]true"),
                                  qsl("sharedEdit=[4]0:0:52:2"),
                                  qsl("sharedEdit.edit=[4]0:0:1:1"),
                                  qsl("sharedEdit2=[1]true"),
                                  qsl("sharedEdit2=[4]0:0:2:32"),
                                  qsl("mini.before=[2]nil:command-line name 'csMini' not found"),
                                  qsl("mini.sheet=[1]QPlainTextEdit{background-color: rgb(0, 0, 0);}"),
                                  qsl("mini.set=[1]true"),
                                  qsl("mini.get=[1]min-height: 300px; min-width: 300px"),
                                  qsl("mini.disabled=[1]true"),
                                  qsl("mini.disabledGet=[1]min-height: 30px"),
                                  qsl("mini.reenabled=[1]true"),
                                  qsl("mini.reenabledGet=[1]min-height: 30px"),
                                  qsl("displaced.get=[1]QPlainTextEdit{background-color: rgb(0, 0, 0);}"),
                                  qsl("window.set=[1]true"),
                                  qsl("window.get=[1]min-height: 30px"),
                                  qsl("buffer.set=[1]true"),
                                  qsl("buffer.get=[1]color: red"),
                                  qsl("csFrame.sheet=[1]true"),
                                  qsl("csFrame.move=[1]true"),
                                  qsl("csFrame.type=[1]commandline"),
                                  qsl("csFrame=[4]0:0:2002:2002"),
                                  qsl("csFrame.resized=[4]0:0:2002:2002"),
                                  qsl("csFrame.moveOut=[1]true"),
                                  qsl("csFrame.out=[4]1:1:2002:2002"),
                                  qsl("csFrame.outResized=[4]1:1:2002:2002"),
                                  qsl("csFrame.outResizedLarge=[4]1:1:2002:2002"),
                                  qsl("csFrame.get=[1]min-height: 2000px; min-width: 2000px"),
                                  qsl("csFrameMain.sheet=[1]true"),
                                  qsl("csFrameMain.move=[1]true"),
                                  qsl("csFrameMain.type=[1]commandline"),
                                  qsl("csFrameMain=[4]0:0:2002:2002"),
                                  qsl("csFrameMain.resized=[4]0:0:2002:2002"),
                                  qsl("csFrameMain.moveOut=[1]true"),
                                  qsl("csFrameMain.out=[4]1:1:2002:2002"),
                                  qsl("csFrameMain.outResized=[4]1:1:2002:2002"),
                                  qsl("csFrameMain.outResizedLarge=[4]1:1:2002:2002"),
                                  qsl("csFrameMain.get=[1]min-height: 2000px; min-width: 2000px"),
                                  qsl("csFrameMax.sheet=[1]true"),
                                  qsl("csFrameMax.move=[1]true"),
                                  qsl("csFrameMax.type=[1]commandline"),
                                  qsl("csFrameMax.moveOut=[1]true"),
                                  qsl("csFrameMax.out=[4]1:1:52:22"),
                                  qsl("csFrameMax.outResized=[4]1:1:5:5"),
                                  qsl("csFrameMax.outResizedLarge=[4]1:1:52:22"),
                                  qsl("csFrameMax.get=[1]max-height: 20px; max-width: 50px")};
        const QStringList actual = luaGlobalString(host, "headlessCmdLineStyleAnswers").split(QLatin1Char('\n'));
        QStringList mismatches;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                mismatches << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        const QStringList mainAnswers{qsl("main=[1]true"), qsl("mainGet=[1]min-height: 5px"), qsl("noName=[1]true"), qsl("empty=[1]true"), qsl("nilGet=[1]min-height: 5px")};
        QCOMPARE(luaGlobalString(host, "headlessCmdLineStyleMainAnswers"), mainAnswers.join(QLatin1Char('\n')));

        // A new command line's sheet is built from the profile's command line background as TCommandLine's constructor
        // builds it, which the GUI profile's black leaves untested
        host->mCommandLineBgColor = QColor(10, 20, 30);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createCommandLine("csColour", 0, 0, 10, 10)
createMiniConsole("csColourMini", 0, 0, 10, 10)
enableCommandLine("csColourMini")
headlessCmdLineColours = getCmdLineStyleSheet("csColour") .. "|" .. getCmdLineStyleSheet("csColourMini")
)lua")));
        QCOMPARE(luaGlobalString(host, "headlessCmdLineColours"), qsl("QPlainTextEdit{background-color: rgb(10, 20, 30);}|QPlainTextEdit{background-color: rgb(10, 20, 30);}"));
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessMainCommandLineContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Main-Command-Line");
        // Opened rather than added, as saveProfile() refuses a profile that was never loaded
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->mLoadedOk, qPrintable(host->mProfileLoadError));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessMainCmdLine = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      -- Bytes a line of answers cannot hold as they are, by number
      values[i] = tostring((select(i, ...))):gsub("[%c\128-\255]", function(byte)
        return "\\" .. byte:byte()
      end)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local startSheet = getCmdLineStyleSheet()
  local startHistory = getSaveCommandHistory()
  clearCmdLine()

  step("get", getCmdLine)
  step("print", printCmdLine, "one")
  step("get.print", getCmdLine)
  step("get.main", getCmdLine, "main")
  step("get.empty", getCmdLine, "")
  step("printMain", printCmdLine, "main", "a\tb\r\nc\rd\194\160e\226\128\168f\226\128\169g\0h")
  step("get.printMain", getCmdLine)
  step("printEmpty", printCmdLine, "", "two")
  step("get.printEmpty", getCmdLine, "main")
  step("printNumber", printCmdLine, 42)
  step("get.printNumber", getCmdLine)
  step("append", appendCmdLine, " more")
  step("appendMain", appendCmdLine, "main", "\194\160x\n")
  step("appendEmpty", appendCmdLine, "", "!")
  step("get.append", getCmdLine)
  step("select", selectCmdLineText)
  step("selectMain", selectCmdLineText, "main")
  step("selectEmpty", selectCmdLineText, "")
  step("get.select", getCmdLine)
  step("clear", clearCmdLine)
  step("get.clear", getCmdLine)
  step("appendToEmpty", appendCmdLine, "y")
  step("get.appendToEmpty", getCmdLine)
  step("clearMain", clearCmdLine, "main")
  step("get.clearMain", getCmdLine)
  printCmdLine("z")
  step("clearEmpty", clearCmdLine, "")
  step("get.clearEmpty", getCmdLine)
  step("send", sendCmdLine, "sent\194\160line\nnext")
  step("get.send", getCmdLine)
  clearCmdLine()

  for _, name in ipairs({"", "main"}) do
    step("suggest." .. name, addCmdLineSuggestion, name, "word")
    step("unsuggest." .. name, removeCmdLineSuggestion, name, "word")
    step("clearSuggestions." .. name, clearCmdLineSuggestions, name)
    step("blacklist." .. name, addCmdLineBlacklist, name, "word")
    step("unblacklist." .. name, removeCmdLineBlacklist, name, "word")
    step("clearBlacklist." .. name, clearCmdLineBlacklist, name)
  end
  step("suggest", addCmdLineSuggestion, "word")
  step("unsuggest", removeCmdLineSuggestion, "word")
  step("clearSuggestions", clearCmdLineSuggestions)
  step("blacklist", addCmdLineBlacklist, "word")
  step("unblacklist", removeCmdLineBlacklist, "word")
  step("clearBlacklist", clearCmdLineBlacklist)

  step("menu", addCommandLineMenuEvent, "mcItem", "mcEvent")
  step("unmenu", removeCommandLineMenuEvent, "mcItem")
  step("unmenuAgain", removeCommandLineMenuEvent, "mcItem")
  step("menuMain", addCommandLineMenuEvent, "main", "mcItem", "mcEvent")
  step("unmenuEmpty", removeCommandLineMenuEvent, "", "mcItem")
  step("menuEmpty", addCommandLineMenuEvent, "", "mcOther", "mcEvent")
  step("menuEmptyAgain", addCommandLineMenuEvent, "", "mcOther", "mcEvent2")
  step("unmenuMain", removeCommandLineMenuEvent, "main", "mcOther")
  step("unmenuMainAgain", removeCommandLineMenuEvent, "main", "mcOther")
  createCommandLine("mcLine", 0, 0, 10, 10)
  addCommandLineMenuEvent("mcLine", "mcShared", "mcEvent")
  step("unmenuMainShared", removeCommandLineMenuEvent, "main", "mcShared")
  step("unmenuLineShared", removeCommandLineMenuEvent, "mcLine", "mcShared")
  addCommandLineMenuEvent("main", "mcMainOnly", "mcEvent")
  step("unmenuLineMainOnly", removeCommandLineMenuEvent, "mcLine", "mcMainOnly")
  step("unmenuMainOnly", removeCommandLineMenuEvent, "mcMainOnly")
  printCmdLine("mcLine", "sub text")
  printCmdLine("main text")
  step("get.sub", getCmdLine, "mcLine")
  step("get.mainBesideSub", getCmdLine)
  setCmdLineStyleSheet("mcLine", "color: blue;")
  step("sheet.mainBesideSub", getCmdLineStyleSheet)
  setSaveCommandHistory("mcLine", false)
  step("history.mainBesideSub", getSaveCommandHistory)
  deleteCommandLine("mcLine")
  clearCmdLine()

  step("history", getSaveCommandHistory)
  step("historyMain", getSaveCommandHistory, "main")
  step("historyEmpty", getSaveCommandHistory, "")
  step("setHistory", setSaveCommandHistory, false)
  step("history.off", getSaveCommandHistory)
  step("historyMain.off", getSaveCommandHistory, "main")
  step("setHistoryMain", setSaveCommandHistory, "main")
  step("historyMain.on", getSaveCommandHistory, "main")
  step("setHistoryEmpty", setSaveCommandHistory, "", false)
  step("historyEmpty.off", getSaveCommandHistory, "")
  step("setHistoryNone", setSaveCommandHistory)
  step("history.none", getSaveCommandHistory)
  setSaveCommandHistory("main", startHistory ~= false)

  step("sheet", getCmdLineStyleSheet)
  step("sheetMain", getCmdLineStyleSheet, "main")
  step("sheetEmpty", getCmdLineStyleSheet, "")
  step("sheetNil", getCmdLineStyleSheet, nil)
  step("setSheet", setCmdLineStyleSheet, "QPlainTextEdit { color: red; }")
  step("sheet.set", getCmdLineStyleSheet)
  step("setSheetMain", setCmdLineStyleSheet, "main", "color: green;")
  step("sheet.setMain", getCmdLineStyleSheet, "main")
  step("setSheetEmpty", setCmdLineStyleSheet, "", "")
  step("sheet.setEmpty", getCmdLineStyleSheet, "")
  setCmdLineStyleSheet("main", startSheet or "")

  printCmdLine("kept")
  step("echo.fed", feedTelnet, "<T_IAC><T_WILL><O_ECHO>")
  step("echo.get", getCmdLine)
  step("echo.print", printCmdLine, "secret")
  step("echo.get.print", getCmdLine)
  step("echo.append", appendCmdLine, "main", "more")
  step("echo.clear", clearCmdLine)
  step("echo.select", selectCmdLineText)
  step("echo.send", sendCmdLine, "sent")
  step("echo.get.after", getCmdLine)
  step("echo.printMain", printCmdLine, "main", "x")
  step("echo.ended", feedTelnet, "<T_IAC><T_WONT><O_ECHO>")
  step("echo.get.ended", getCmdLine)
  step("echo.print.ended", printCmdLine, "after")
  step("echo.get.printEnded", getCmdLine)
  clearCmdLine()
  headlessMainCmdLineAnswers = table.concat(answers, "\n")
end)
headlessMainCmdLine = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMainCmdLine"), qsl("ok"));
        const QStringList answers{qsl("get=[1]"),
                                  qsl("print=[0]"),
                                  qsl("get.print=[1]one"),
                                  qsl("get.main=[1]one"),
                                  qsl("get.empty=[1]one"),
                                  qsl("printMain=[0]"),
                                  qsl("get.printMain=[1]a\\9b\\10c\\10d e\\10f\\10g"),
                                  qsl("printEmpty=[0]"),
                                  qsl("get.printEmpty=[1]two"),
                                  qsl("printNumber=[0]"),
                                  qsl("get.printNumber=[1]42"),
                                  qsl("append=[0]"),
                                  qsl("appendMain=[0]"),
                                  qsl("appendEmpty=[0]"),
                                  qsl("get.append=[1]42 more x\\10!"),
                                  qsl("select=[1]true"),
                                  qsl("selectMain=[1]true"),
                                  qsl("selectEmpty=[1]true"),
                                  qsl("get.select=[1]42 more x\\10!"),
                                  qsl("clear=[0]"),
                                  qsl("get.clear=[1]"),
                                  qsl("appendToEmpty=[0]"),
                                  qsl("get.appendToEmpty=[1]y"),
                                  qsl("clearMain=[0]"),
                                  qsl("get.clearMain=[1]"),
                                  qsl("clearEmpty=[0]"),
                                  qsl("get.clearEmpty=[1]"),
                                  qsl("send=[1]true"),
                                  qsl("get.send=[1]sent line\\10next"),
                                  qsl("suggest.=[0]"),
                                  qsl("unsuggest.=[0]"),
                                  qsl("clearSuggestions.=[0]"),
                                  qsl("blacklist.=[0]"),
                                  qsl("unblacklist.=[0]"),
                                  qsl("clearBlacklist.=[0]"),
                                  qsl("suggest.main=[0]"),
                                  qsl("unsuggest.main=[0]"),
                                  qsl("clearSuggestions.main=[0]"),
                                  qsl("blacklist.main=[0]"),
                                  qsl("unblacklist.main=[0]"),
                                  qsl("clearBlacklist.main=[0]"),
                                  qsl("suggest=[0]"),
                                  qsl("unsuggest=[0]"),
                                  qsl("clearSuggestions=[0]"),
                                  qsl("blacklist=[0]"),
                                  qsl("unblacklist=[0]"),
                                  qsl("clearBlacklist=[0]"),
                                  qsl("menu=[1]true"),
                                  qsl("unmenu=[1]true"),
                                  qsl("unmenuAgain=[2]nil:cannot remove 'mcItem', menu item does not exist"),
                                  qsl("menuMain=[1]true"),
                                  qsl("unmenuEmpty=[1]true"),
                                  qsl("menuEmpty=[1]true"),
                                  qsl("menuEmptyAgain=[1]true"),
                                  qsl("unmenuMain=[1]true"),
                                  qsl("unmenuMainAgain=[2]nil:cannot remove 'mcOther', menu item does not exist"),
                                  qsl("unmenuMainShared=[2]nil:cannot remove 'mcShared', menu item does not exist"),
                                  qsl("unmenuLineShared=[1]true"),
                                  qsl("unmenuLineMainOnly=[2]nil:cannot remove 'mcMainOnly', menu item does not exist"),
                                  qsl("unmenuMainOnly=[1]true"),
                                  qsl("get.sub=[1]sub text"),
                                  qsl("get.mainBesideSub=[1]main text"),
                                  qsl("sheet.mainBesideSub=[1]"),
                                  qsl("history.mainBesideSub=[2]true:enabled (500 lines will be saved)"),
                                  qsl("history=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyMain=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyEmpty=[2]true:enabled (500 lines will be saved)"),
                                  qsl("setHistory=[1]true"),
                                  qsl("history.off=[2]false:disabled"),
                                  qsl("historyMain.off=[2]false:disabled"),
                                  qsl("setHistoryMain=[1]true"),
                                  qsl("historyMain.on=[2]true:enabled (500 lines will be saved)"),
                                  qsl("setHistoryEmpty=[1]true"),
                                  qsl("historyEmpty.off=[2]false:disabled"),
                                  qsl("setHistoryNone=[1]true"),
                                  qsl("history.none=[2]true:enabled (500 lines will be saved)"),
                                  qsl("sheet=[1]"),
                                  qsl("sheetMain=[1]"),
                                  qsl("sheetEmpty=[1]"),
                                  qsl("sheetNil=[1]"),
                                  qsl("setSheet=[1]true"),
                                  qsl("sheet.set=[1]QPlainTextEdit { color: red; }"),
                                  qsl("setSheetMain=[1]true"),
                                  qsl("sheet.setMain=[1]color: green;"),
                                  qsl("setSheetEmpty=[1]true"),
                                  qsl("sheet.setEmpty=[1]"),
                                  qsl("echo.fed=[1]true"),
                                  qsl("echo.get=[1]kept"),
                                  qsl("echo.print=[0]"),
                                  qsl("echo.get.print=[1]kept"),
                                  qsl("echo.append=[0]"),
                                  qsl("echo.clear=[0]"),
                                  qsl("echo.select=[1]true"),
                                  qsl("echo.send=[1]true"),
                                  qsl("echo.get.after=[1]kept"),
                                  qsl("echo.printMain=[0]"),
                                  qsl("echo.ended=[1]true"),
                                  qsl("echo.get.ended=[1]kept"),
                                  qsl("echo.print.ended=[0]"),
                                  qsl("echo.get.printEnded=[1]after")};
        const QStringList actual = luaGlobalString(host, "headlessMainCmdLineAnswers").split(QLatin1Char('\n'));
        QStringList mismatches;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                mismatches << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        // The main command line writes its setting on an end of session save, as its TCommandLine does. The key is
        // set the other way first, so the save is seen to write it.
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("setSaveCommandHistory(false)")));
        host->writeProfileIniData(qsl("CommandLines/SaveHistory/main"), qsl("true"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY(std::get<0>(host->saveProfile()));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/main")), qsl("false"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        // A profile opened again starts from what was saved, as TCommandLine's constructor reads it
        HostManager::self()->deleteHost(hostname);
        host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open again.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessMainHistoryReopened = tostring(getSaveCommandHistory())")));
        QCOMPARE(luaGlobalString(host, "headlessMainHistoryReopened"), qsl("false"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the main command line created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua chunks run in the GUI self-test profile, with the profile's name taken out
    void test_headlessMapWidgetContract()
    {
        // Each chunk runs between these, as it did in the GUI
        const QString prelude = qsl(R"lua(
headlessMapWidget = "not run"
local ok, err = pcall(function()
  local profile = getProfileName():gsub("%p", "%%%0")
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...))):gsub(profile, "<profile>")
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function count(f, ...)
    return select("#", f(...))
  end
  mapWidgetOpenEvents = 0
  function headlessMapWidgetOpened() mapWidgetOpenEvents = mapWidgetOpenEvents + 1 end
  local handler = registerAnonymousEventHandler("mapOpenEvent", "headlessMapWidgetOpened")
  local function events() return mapWidgetOpenEvents end
  local function playerRoom()
    local path = getMudletHomeDir() .. "/player room.json"
    saveJsonMap(path)
    local file = io.open(path)
    local text = file:read("*a")
    file:close()
    return text:match([["playerRoomColors"%s*:%s*(%b[])]]):gsub("%s", ""), text:match([["playerRoomStyle"%s*:%s*(%d+)]])
  end
)lua");
        const QString epilogue = qsl(R"lua(
  killAnonymousEventHandler(handler)
  headlessMapWidgetAnswers = table.concat(answers, "\n")
end)
headlessMapWidget = ok and "ok" or tostring(err)
)lua");
        Host* host = nullptr;
        QString hostname;
        int chunks = 0;
        // Each on a profile of its own, so none finds a map another saved; the answers that differ from the GUI's come back
        const auto mismatches = [&](const QString& chunk, const QStringList& answers) {
            if (host) {
                HostManager::self()->deleteHost(hostname);
            }
            hostname = qsl("Test-Headless-Host-Map-Widget-%1").arg(++chunks);
            host = HostManager::self()->addHost(hostname, QString(), QString(), QString()) ? HostManager::self()->getHost(hostname) : nullptr;
            if (!host) {
                return QStringList{qsl("the profile could not be made")};
            }
            // For moveMapWidget() and resizeMapWidget(), which GUIUtils.lua builds on openMapWidget()
            host->getLuaInterpreter()->loadGlobal();
            if (!host->getLuaInterpreter()->compileAndExecuteScript(prelude + chunk + epilogue) || luaGlobalString(host, "headlessMapWidget") != qsl("ok")) {
                return QStringList{qsl("the chunk did not run: %1").arg(luaGlobalString(host, "headlessMapWidget"))};
            }
            const QStringList actual = luaGlobalString(host, "headlessMapWidgetAnswers").split(QLatin1Char('\n'));
            QStringList different;
            for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
                if (actual.value(i) != answers.value(i)) {
                    different << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
                }
            }
            return different;
        };

        const QStringList answers1{qsl("rooms.start=[1]0"),
                                   qsl("save=[1]true"),
                                   qsl("rooms.saved=[1]0"),
                                   qsl("title.none=[2]nil:no floating/dockable type map window found"),
                                   qsl("setTitle.none=[2]nil:no floating/dockable type map window found"),
                                   qsl("geometry.none=[2]nil:no floating/dockable type map window found"),
                                   qsl("close.none=[2]nil:no map widget found to close"),
                                   qsl("setWindow.none=[2]nil:element 'mapper' not found"),
                                   qsl("raise.none=[1]false"),
                                   qsl("embedded.noWindow=[2]false:window 'noSuchWindow' not found"),
                                   qsl("embedded.scrollBox=[2]false:window 'mwBox' not found"),
                                   qsl("embedded.miniConsole=[2]false:window 'mwMini' not found"),
                                   qsl("open.badType=raised:openMapWidget: bad argument #1 type (area as string expected, got boolean!)"),
                                   qsl("events.none=[1]0"),
                                   qsl("open.bogus=[2]nil:docking option \"bogus\" not available. available docking options are \"t\" top, \"b\" bottom, \"r\" right, \"l\" left and \"f\" floating"),
                                   qsl("events.bogus=[1]1"),
                                   qsl("room.open=[1]Saved room"),
                                   qsl("title.bogus=[1]Map - <profile>"),
                                   qsl("open=[1]true"),
                                   qsl("events.open=[1]1"),
                                   qsl("title=[1]Map - <profile>"),
                                   qsl("setTitle=[1]true"),
                                   qsl("title.set=[1]Custom map"),
                                   qsl("setTitle.none=[1]true"),
                                   qsl("title.reset=[1]Map - <profile>"),
                                   qsl("setTitle.again=[1]true"),
                                   qsl("setTitle.empty=[1]true"),
                                   qsl("title.empty=[1]Map - <profile>"),
                                   qsl("geometry.docked.count=[1]4"),
                                   qsl("float=[1]true"),
                                   qsl("geometry.float=[4]10:20:400:300"),
                                   qsl("move.type=[1]function"),
                                   qsl("move=[0]"),
                                   qsl("geometry.move=[4]30:40:400:300"),
                                   qsl("resize=[0]"),
                                   qsl("geometry.resize=[4]30:40:500:350"),
                                   qsl("float.F=[1]true"),
                                   qsl("geometry.F=[4]30:40:500:350"),
                                   qsl("float.floating=[1]true"),
                                   qsl("float.noChange=[1]true"),
                                   qsl("geometry.noChange=[4]30:40:500:350"),
                                   qsl("float.posOnly=[1]true"),
                                   qsl("geometry.posOnly=[4]50:60:500:350"),
                                   qsl("float.halfSize=[1]true"),
                                   qsl("geometry.halfSize=[4]70:80:500:350"),
                                   qsl("float.halfPos=[1]true"),
                                   qsl("geometry.halfPos=[4]70:80:450:250"),
                                   qsl("float.xOnly=[1]true"),
                                   qsl("geometry.xOnly=[4]70:80:450:250"),
                                   qsl("float.widthOnly=[1]true"),
                                   qsl("geometry.widthOnly=[4]70:80:450:250"),
                                   qsl("dock.r=[1]true"),
                                   qsl("geometry.dock.r.count=[1]4"),
                                   qsl("dock.Right=[1]true"),
                                   qsl("geometry.dock.Right.count=[1]4"),
                                   qsl("dock.l=[1]true"),
                                   qsl("geometry.dock.l.count=[1]4"),
                                   qsl("dock.left=[1]true"),
                                   qsl("geometry.dock.left.count=[1]4"),
                                   qsl("dock.t=[1]true"),
                                   qsl("geometry.dock.t.count=[1]4"),
                                   qsl("dock.top=[1]true"),
                                   qsl("geometry.dock.top.count=[1]4"),
                                   qsl("dock.b=[1]true"),
                                   qsl("geometry.dock.b.count=[1]4"),
                                   qsl("dock.bottom=[1]true"),
                                   qsl("geometry.dock.bottom.count=[1]4"),
                                   qsl("dock.bad=[2]nil:docking option \"middle\" not available. available docking options are \"t\" top, \"b\" bottom, \"r\" right, \"l\" left and \"f\" floating"),
                                   qsl("float.again=[1]true"),
                                   qsl("geometry.again=[4]15:25:410:310"),
                                   qsl("setWindow.mapper=[2]nil:element 'mapper' is the map in a floating/dockable window and may not be moved"),
                                   qsl("setWindow.into=[2]nil:window 'mapper' is the map in a floating/dockable window and may not receive other elements"),
                                   qsl("raise=[1]false"),
                                   qsl("lower=[1]false"),
                                   qsl("embedded.overShown=[2]false:cannot create mapper. Do you already use a map window?"),
                                   qsl("events.overShown=[1]1"),
                                   qsl("close=[1]true"),
                                   qsl("close.again=[2]nil:map widget already closed"),
                                   qsl("title.closed=[2]nil:no floating/dockable type map window found"),
                                   qsl("setTitle.closed=[2]nil:no floating/dockable type map window found"),
                                   qsl("geometry.closed=[2]nil:no floating/dockable type map window found"),
                                   qsl("setWindow.closed=[2]nil:element 'mapper' is the map in a floating/dockable window and may not be moved"),
                                   qsl("reopen=[1]true"),
                                   qsl("events.reopen=[1]1"),
                                   qsl("title.reopen=[1]Kept"),
                                   qsl("geometry.reopen=[4]15:25:410:310"),
                                   qsl("close.reopen=[1]true"),
                                   qsl("reopen.bogus=[2]nil:docking option \"bogus\" not available. available docking options are \"t\" top, \"b\" bottom, \"r\" right, \"l\" left and \"f\" floating"),
                                   qsl("title.reopenBogus=[1]Kept"),
                                   qsl("close.reopenBogus=[1]true"),
                                   qsl("reopen.float=[1]true"),
                                   qsl("geometry.reopenFloat=[4]5:6:420:320"),
                                   qsl("close.reopenFloat=[1]true"),
                                   qsl("embedded=[1]true"),
                                   qsl("events.embedded=[1]2"),
                                   qsl("room.embedded=[1]Saved room"),
                                   qsl("close.embedded=[2]nil:no map widget found to close"),
                                   qsl("open.embedded=[2]nil:cannot create map widget. Do you already use an embedded mapper?"),
                                   qsl("open.embedded.float=[2]nil:cannot create map widget. Do you already use an embedded mapper?"),
                                   qsl("title.embedded=[2]nil:no floating/dockable type map window found"),
                                   qsl("setTitle.embedded=[2]nil:no floating/dockable type map window found"),
                                   qsl("geometry.embedded=[2]nil:no floating/dockable type map window found"),
                                   qsl("setWindow.embedded=[1]true"),
                                   qsl("raise.embedded=[1]true"),
                                   qsl("lower.embedded=[1]true"),
                                   qsl("embedded.again=[1]true"),
                                   qsl("embedded.noWindow.again=[2]false:window 'noSuchWindow' not found"),
                                   qsl("embedded.userWindow=[1]true"),
                                   qsl("embedded.scrollBox.again=[2]false:window 'mwBox' not found"),
                                   qsl("setWindow.embedded.userWindow=[1]true"),
                                   qsl("setWindow.embedded.box=[1]true"),
                                   qsl("raise.embedded.upper=[1]true"),
                                   qsl("visible.embedded=[2]nil:window \"mapper\" not found"),
                                   qsl("windowGeometry.embedded=[2]nil:window \"mapper\" not found"),
                                   qsl("events.again=[1]2"),
                                   qsl("load.embedded=[1]false"),
                                   qsl("events.load.embedded=[1]2"),
                                   qsl("title.load.embedded=[2]nil:no floating/dockable type map window found"),
                                   qsl("close.load.embedded=[2]nil:no map widget found to close")};
        const QStringList different1 = mismatches(qsl(R"lua(
  createLabel("mwLabel", 0, 0, 10, 10, 1)
  openUserWindow("mwWindow")
  createScrollBox("mwBox", 0, 0, 50, 50)
  createMiniConsole("mwMini", 0, 0, 50, 50)
  local function roomCount()
    local rooms = 0
    for _ in pairs(getRooms()) do rooms = rooms + 1 end
    return rooms
  end
  step("rooms.start", roomCount)
  local savedRoom = createRoomID()
  addRoom(savedRoom)
  setRoomName(savedRoom, "Saved room")
  step("save", saveMap)
  deleteRoom(savedRoom)
  step("rooms.saved", roomCount)

  step("title.none", getMapWindowTitle)
  step("setTitle.none", setMapWindowTitle, "Never")
  step("geometry.none", getMapWidgetGeometry)
  step("close.none", closeMapWidget)
  step("setWindow.none", setWindow, "main", "mapper", 0, 0, true)
  step("raise.none", raiseWindow, "mapper")
  step("embedded.noWindow", createMapper, "noSuchWindow", 0, 0, 10, 10)
  step("embedded.scrollBox", createMapper, "mwBox", 0, 0, 10, 10)
  step("embedded.miniConsole", createMapper, "mwMini", 0, 0, 10, 10)
  step("open.badType", openMapWidget, true)
  step("events.none", events)

  step("open.bogus", openMapWidget, "bogus")
  step("events.bogus", events)
  step("room.open", getRoomName, savedRoom)
  step("title.bogus", getMapWindowTitle)
  step("open", openMapWidget)
  step("events.open", events)
  step("title", getMapWindowTitle)
  step("setTitle", setMapWindowTitle, "Custom map")
  step("title.set", getMapWindowTitle)
  step("setTitle.none", setMapWindowTitle)
  step("title.reset", getMapWindowTitle)
  step("setTitle.again", setMapWindowTitle, "Second")
  step("setTitle.empty", setMapWindowTitle, "")
  step("title.empty", getMapWindowTitle)
  step("geometry.docked.count", count, getMapWidgetGeometry)

  step("float", openMapWidget, 10, 20, 400, 300)
  step("geometry.float", getMapWidgetGeometry)
  step("move.type", type, moveMapWidget)
  step("move", moveMapWidget, 30, 40)
  step("geometry.move", getMapWidgetGeometry)
  step("resize", resizeMapWidget, 500, 350)
  step("geometry.resize", getMapWidgetGeometry)
  step("float.F", openMapWidget, "F")
  step("geometry.F", getMapWidgetGeometry)
  step("float.floating", openMapWidget, "floating")
  step("float.noChange", openMapWidget, -1, -1, -1, -1)
  step("geometry.noChange", getMapWidgetGeometry)
  step("float.posOnly", openMapWidget, 50, 60)
  step("geometry.posOnly", getMapWidgetGeometry)
  step("float.halfSize", openMapWidget, 70, 80, -1, 200)
  step("geometry.halfSize", getMapWidgetGeometry)
  step("float.halfPos", openMapWidget, -1, 90, 450, 250)
  step("geometry.halfPos", getMapWidgetGeometry)
  step("float.xOnly", openMapWidget, 35, -1)
  step("geometry.xOnly", getMapWidgetGeometry)
  step("float.widthOnly", openMapWidget, -1, -1, 300, -1)
  step("geometry.widthOnly", getMapWidgetGeometry)
  for _, area in ipairs({"r", "Right", "l", "left", "t", "top", "b", "bottom"}) do
    step("dock." .. area, openMapWidget, area)
    step("geometry.dock." .. area .. ".count", count, getMapWidgetGeometry)
  end
  step("dock.bad", openMapWidget, "middle")
  step("float.again", openMapWidget, 15, 25, 410, 310)
  step("geometry.again", getMapWidgetGeometry)
  step("setWindow.mapper", setWindow, "main", "mapper", 0, 0, true)
  step("setWindow.into", setWindow, "mapper", "mwLabel", 0, 0, true)
  step("raise", raiseWindow, "mapper")
  step("lower", lowerWindow, "mapper")
  step("embedded.overShown", createMapper, 0, 0, 300, 300)
  step("events.overShown", events)
  setMapWindowTitle("Kept")

  step("close", closeMapWidget)
  step("close.again", closeMapWidget)
  step("title.closed", getMapWindowTitle)
  step("setTitle.closed", setMapWindowTitle, "Closed")
  step("geometry.closed", getMapWidgetGeometry)
  step("setWindow.closed", setWindow, "main", "mapper", 0, 0, true)
  step("reopen", openMapWidget)
  step("events.reopen", events)
  step("title.reopen", getMapWindowTitle)
  step("geometry.reopen", getMapWidgetGeometry)
  step("close.reopen", closeMapWidget)
  step("reopen.bogus", openMapWidget, "bogus")
  step("title.reopenBogus", getMapWindowTitle)
  step("close.reopenBogus", closeMapWidget)
  step("reopen.float", openMapWidget, 5, 6, 420, 320)
  step("geometry.reopenFloat", getMapWidgetGeometry)
  step("close.reopenFloat", closeMapWidget)

  deleteRoom(savedRoom)
  step("embedded", createMapper, 0, 0, 300, 300)
  step("events.embedded", events)
  step("room.embedded", getRoomName, savedRoom)
  step("close.embedded", closeMapWidget)
  step("open.embedded", openMapWidget)
  step("open.embedded.float", openMapWidget, 1, 2, 300, 300)
  step("title.embedded", getMapWindowTitle)
  step("setTitle.embedded", setMapWindowTitle, "Embedded")
  step("geometry.embedded", getMapWidgetGeometry)
  step("setWindow.embedded", setWindow, "main", "mapper", 0, 0, true)
  step("raise.embedded", raiseWindow, "mapper")
  step("lower.embedded", lowerWindow, "mapper")
  step("embedded.again", createMapper, "main", 10, 10, 200, 200)
  step("embedded.noWindow.again", createMapper, "noSuchWindow", 0, 0, 10, 10)
  step("embedded.userWindow", createMapper, "mwWindow", 0, 0, 10, 10)
  step("embedded.scrollBox.again", createMapper, "mwBox", 0, 0, 10, 10)
  step("setWindow.embedded.userWindow", setWindow, "mwWindow", "mapper", 0, 0, true)
  step("setWindow.embedded.box", setWindow, "mwBox", "MAPPER", 0, 0, false)
  step("raise.embedded.upper", raiseWindow, "Mapper")
  step("visible.embedded", windowVisible, "mapper")
  step("windowGeometry.embedded", getWindowGeometry, "mapper")
  step("events.again", events)
  step("load.embedded", loadMap, getMudletHomeDir() .. "/no such map.dat")
  step("events.load.embedded", events)
  step("title.load.embedded", getMapWindowTitle)
  step("close.load.embedded", closeMapWidget)

  deleteLabel("mwLabel")
  deleteScrollBox("mwBox")
)lua"),
                                                  answers1);
        QVERIFY2(different1.isEmpty(), qPrintable(different1.join(QChar::LineFeed)));
        // Lua hands createMapper() the main window as "", but the view takes "main" in any case too, as TMainConsole does
        QVERIFY(host->consoleFrontend()->createMapper(qsl("MAIN"), 0, 0, 10, 10).first);
        // The chunk moved the mapper into the scroll box it then deleted, which takes the mapper with it once deferred
        // deletes run, as TMainConsole::deleteScrollBox() does its dlgMapper. A spec cannot wait for those.
        QVERIFY(host->consoleFrontend()->raiseWindow(qsl("mapper")));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!host->consoleFrontend()->raiseWindow(qsl("mapper")));
        QVERIFY(host->openMapWidget(QString(), -1, -1, -1, -1).first);

        // A map load makes the map widget when there is no mapper, as the GUI's does
        const QStringList answers2{qsl("title.none=[2]nil:no floating/dockable type map window found"),
                                   qsl("load=[1]false"),
                                   qsl("events.load=[1]1"),
                                   qsl("title.load=[1]Map - <profile>"),
                                   qsl("geometry.load.count=[1]4"),
                                   qsl("load.again=[1]false"),
                                   qsl("events.again=[1]1"),
                                   qsl("float=[1]true"),
                                   qsl("close=[1]true"),
                                   qsl("load.closed=[1]false"),
                                   qsl("events.closed=[1]1"),
                                   qsl("title.closed=[2]nil:no floating/dockable type map window found"),
                                   qsl("close.closed=[2]nil:map widget already closed"),
                                   qsl("reopen=[1]true"),
                                   qsl("events.reopen=[1]1"),
                                   qsl("geometry.reopen=[4]10:20:400:300")};
        const QStringList different2 = mismatches(qsl(R"lua(
  local missing = getMudletHomeDir() .. "/no such map.dat"
  step("title.none", getMapWindowTitle)
  step("load", loadMap, missing)
  step("events.load", events)
  step("title.load", getMapWindowTitle)
  step("geometry.load.count", count, getMapWidgetGeometry)
  step("load.again", loadMap, missing)
  step("events.again", events)
  step("float", openMapWidget, 10, 20, 400, 300)
  step("close", closeMapWidget)
  step("load.closed", loadMap, missing)
  step("events.closed", events)
  step("title.closed", getMapWindowTitle)
  step("close.closed", closeMapWidget)
  step("reopen", openMapWidget)
  step("events.reopen", events)
  step("geometry.reopen", getMapWidgetGeometry)

)lua"),
                                                  answers2);
        QVERIFY2(different2.isEmpty(), qPrintable(different2.join(QChar::LineFeed)));
        // Not a GUI answer, which comes from the layout: with none to dock it into, the model gives it the main window's
        // size, as it does a docked user window
        const QRect mainWindow(QPoint(0, 0), host->mainWindowSize().value_or(QSize()));
        QVERIFY(!mainWindow.isEmpty());
        QVERIFY(host->openMapWidget(qsl("r"), -1, -1, -1, -1).first);
        QCOMPARE(host->mapWidgetGeometry(), std::optional<QRect>(mainWindow));

        // A map widget made with a map loaded keeps that map, as the GUI's does
        const QStringList answers3{qsl("save=[1]true"),
                                   qsl("open=[1]true"),
                                   qsl("events.open=[1]1"),
                                   qsl("room.saved=[1]false"),
                                   qsl("room.kept=[1]Kept room"),
                                   qsl("playerRoom.open=[2][{\"color24RGB\":[255,0,0]},{\"color24RGB\":[255,255,255]}]:0")};
        const QStringList different3 = mismatches(qsl(R"lua(
  local saved = createRoomID()
  addRoom(saved)
  setRoomName(saved, "Saved room")
  step("save", saveMap)
  deleteRoom(saved)
  local kept = saved + 1
  addRoom(kept)
  setRoomName(kept, "Kept room")
  step("open", openMapWidget)
  step("events.open", events)
  step("room.saved", roomExists, saved)
  step("room.kept", getRoomName, kept)
  step("playerRoom.open", playerRoom)

)lua"),
                                                  answers3);
        QVERIFY2(different3.isEmpty(), qPrintable(different3.join(QChar::LineFeed)));
        // Made docked, it starts at that size too
        QCOMPARE(host->mapWidgetGeometry(), std::optional<QRect>(QRect(QPoint(0, 0), host->mainWindowSize().value_or(QSize()))));

        // An XML map import makes the map widget when there is no mapper, as the GUI's does
        const QStringList answers4{qsl("title.none=[2]nil:no floating/dockable type map window found"),
                                   qsl("import=[1]true"),
                                   qsl("events.import=[1]1"),
                                   qsl("title.import=[1]Map - <profile>"),
                                   qsl("room.import=[1]Headless Xml Room"),
                                   qsl("import.again=[1]true"),
                                   qsl("events.again=[1]1")};
        const QStringList different4 = mismatches(qsl(R"lua(
  local path = getMudletHomeDir() .. "/headless.xml"
  local file = io.open(path, "w")
  file:write([[<?xml version="1.0" encoding="UTF-8"?>
<map>
 <areas>
  <area id="1" name="Headless Xml Area"/>
 </areas>
 <rooms>
  <room id="3" area="1" title="Headless Xml Room" environment="2">
   <coord x="0" y="0" z="0"/>
  </room>
 </rooms>
</map>
]])
  file:close()
  step("title.none", getMapWindowTitle)
  step("import", loadMap, path)
  step("events.import", events)
  step("title.import", getMapWindowTitle)
  step("room.import", getRoomName, 3)
  step("import.again", loadMap, path)
  step("events.again", events)
)lua"),
                                                  answers4);
        QVERIFY2(different4.isEmpty(), qPrintable(different4.join(QChar::LineFeed)));

        // An embedded mapper also takes the profile's player room style, as the GUI's does
        const QStringList answers5{qsl("embedded=[1]true"), qsl("playerRoom.embedded=[2][{\"color24RGB\":[255,0,0]},{\"color24RGB\":[255,255,255]}]:0"), qsl("raise.embedded=[1]true")};
        const QStringList different5 = mismatches(qsl(R"lua(
  openUserWindow("mwMapperWindow")
  step("embedded", createMapper, "mwMapperWindow", 0, 0, 10, 10)
  step("playerRoom.embedded", playerRoom)
  step("raise.embedded", raiseWindow, "mapper")
)lua"),
                                                  answers5);
        QVERIFY2(different5.isEmpty(), qPrintable(different5.join(QChar::LineFeed)));
        // A reset takes the user window, and the mapper in it, as TMainConsole::resetMainConsole() takes its dock
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY2(host->resetProfile_phase1(), "The profile reset did not start.");
        QTRY_VERIFY(!host->profileResetInProgress());
        QVERIFY(!host->consoleFrontend()->raiseWindow(qsl("mapper")));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the map widget created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua chunks run in the GUI self-test profile, which pumpEvents() lets paint
    void test_headlessMapViewContract()
    {
        // Each chunk runs between these, as it did in the GUI
        const QString prelude = qsl(R"lua(
headlessMapView = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  -- Long enough for the GUI to lay out and paint a map it has just shown
  local function settle() pumpEvents(300) end
  local area = addAreaName("Headless Map View Area")
  local other = addAreaName("Headless Map View Other")
  local names = {[area] = "area", [other] = "other"}
  local function named(id) return names[tonumber(id)] or tostring(id) end
  local changes = {}
  function headlessMapViewAreaChanged(_, shown, previous)
    changes[#changes + 1] = named(shown) .. "<" .. named(previous)
  end
  local areaHandler = registerAnonymousEventHandler("sysMapAreaChanged", "headlessMapViewAreaChanged")
  local function areaChanges()
    local text = table.concat(changes, ",")
    changes = {}
    return text
  end
  local function room(areaId)
    local id = createRoomID()
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, 0, 0, 0)
    return id
  end
  local near = room(area)
  local far = room(other)
)lua");
        const QString epilogue = qsl(R"lua(
  killAnonymousEventHandler(areaHandler)
  headlessMapViewAnswers = table.concat(answers, "\n")
end)
headlessMapView = ok and "ok" or tostring(err)
)lua");
        Host* host = nullptr;
        QString hostname;
        int chunks = 0;
        // Each on a profile of its own; the answers that differ from the GUI's come back
        const auto mismatches = [&](const QString& chunk, const QStringList& answers) {
            if (host) {
                HostManager::self()->deleteHost(hostname);
            }
            hostname = qsl("Test-Headless-Host-Map-View-%1").arg(++chunks);
            host = HostManager::self()->addHost(hostname, QString(), QString(), QString()) ? HostManager::self()->getHost(hostname) : nullptr;
            if (!host) {
                return QStringList{qsl("the profile could not be made")};
            }
            host->getLuaInterpreter()->loadGlobal();
            // With no view, pumpEvents() takes Mudlet for shutting down and pumps nothing. The null view paints on short
            // chains of zero timers, each link a pass, so passes rather than time deliver them however slow a pass is
            lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State*) -> int {
                for (int pass = 0; pass < 10; ++pass) {
                    QCoreApplication::processEvents();
                }
                return 0;
            });
            if (!host->getLuaInterpreter()->compileAndExecuteScript(prelude + chunk + epilogue) || luaGlobalString(host, "headlessMapView") != qsl("ok")) {
                return QStringList{qsl("the chunk did not run: %1").arg(luaGlobalString(host, "headlessMapView"))};
            }
            const QStringList actual = luaGlobalString(host, "headlessMapViewAnswers").split(QLatin1Char('\n'));
            QStringList different;
            for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
                if (actual.value(i) != answers.value(i)) {
                    different << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
                }
            }
            return different;
        };

        // The map widget's 2D map: the area it shows, its zoom and sysMapAreaChanged, before and after it paints
        QStringList answers1{qsl("zoom.none=[2]nil:no active mapper"),
                             qsl("setZoom.none=[2]nil:no active mapper"),
                             qsl("open=[1]true"),
                             qsl("centerview=[1]true"),
                             qsl("zoom.open=[1]20"),
                             qsl("setZoom.open=[2]nil:number 0 is not a valid areaID"),
                             qsl("changes.open=[1]"),
                             qsl("changes.painted=[1]area<-2"),
                             qsl("zoom.painted=[1]20"),
                             qsl("setZoom.painted=[1]true"),
                             qsl("zoom.set=[1]5"),
                             qsl("zoomArea.set=[1]5"),
                             qsl("zoomOther.set=[1]20"),
                             qsl("setZoom.small=[2]nil:zoom 1 is invalid, it must be at least 3"),
                             qsl("setZoom.nan=[2]nil:zoom nan is invalid, it must be a finite number"),
                             qsl("setZoom.noArea=[2]nil:number 9999 is not a valid areaID"),
                             qsl("setZoom.other=[1]true"),
                             qsl("zoom.other=[1]5"),
                             qsl("centerview.far=[1]true"),
                             qsl("zoom.far=[1]5"),
                             qsl("changes.far=[1]other<area"),
                             qsl("zoom.far.painted=[1]8"),
                             qsl("changes.again=[1]"),
                             qsl("close=[1]true"),
                             qsl("centerview.closed=[1]true"),
                             qsl("changes.closed=[1]"),
                             qsl("zoom.closed=[1]8"),
                             qsl("setZoom.closed=[1]true"),
                             qsl("zoomOther.closed=[1]9"),
                             qsl("reopen=[1]true"),
                             qsl("zoom.reopen=[1]9"),
                             qsl("changes.reopen=[1]area<other"),
                             qsl("zoom.reopen.painted=[1]5"),
                             qsl("move=[1]true"),
                             qsl("changes.moved=[1]"),
                             qsl("zoom.moved=[1]5"),
                             qsl("centerview.moved=[1]true"),
                             qsl("changes.recentred=[1]other<area"),
                             qsl("zoom.recentred=[1]9"),
                             qsl("moveBack=[1]true"),
                             qsl("centerview.back=[1]true"),
                             qsl("changes.back=[1]area<other"),
                             qsl("save=[1]true"),
                             qsl("centerview.far2=[1]true"),
                             qsl("changes.far2=[1]other<area"),
                             qsl("zoom.far2=[1]9"),
                             qsl("load=[1]true"),
                             qsl("zoom.load=[1]9"),
                             qsl("changes.load=[1]area<other"),
                             qsl("zoom.load.painted=[1]5"),
                             qsl("saveJson=[1]true"),
                             qsl("centerview.far3=[1]true"),
                             qsl("changes.far3=[1]other<area"),
                             qsl("loadJson=[1]true"),
                             qsl("zoom.loadJson=[1]9"),
                             qsl("changes.loadJson=[1]area<other"),
                             qsl("zoom.loadJson.painted=[1]5"),
                             qsl("centerview.far4=[1]true"),
                             qsl("changes.far4=[1]other<area"),
                             qsl("close.json=[1]true"),
                             qsl("loadJson.closed=[1]true"),
                             qsl("changes.loadJson.closed=[1]"),
                             qsl("title.loadJson.closed=[2]nil:no floating/dockable type map window found"),
                             qsl("reopen.json=[1]true"),
                             qsl("changes.reopen.json=[1]area<other"),
                             qsl("delete=[1]true"),
                             qsl("changes.delete=[1]other<area"),
                             qsl("zoom.delete=[1]9"),
                             qsl("deleteArea=[1]true"),
                             qsl("changes.deleteArea=[1]"),
                             qsl("zoom.deleteArea=[1]20"),
                             qsl("setZoom.deleteArea=[2]nil:number <other> is not a valid areaID")};
#if !defined(INCLUDE_3DMAPPER)
        // Without the 3D mapper, dlgMapper::setMapZoom() hands T2DMap an area of 0, which its refusal names
        answers1.last() = qsl("setZoom.deleteArea=[2]nil:number 0 is not a valid areaID");
#endif
        const QStringList different1 = mismatches(qsl(R"lua(
  step("zoom.none", getMapZoom)
  step("setZoom.none", setMapZoom, 7)
  step("open", openMapWidget)
  step("centerview", centerview, near)
  step("zoom.open", getMapZoom)
  step("setZoom.open", setMapZoom, 5)
  step("changes.open", areaChanges)
  settle()
  step("changes.painted", areaChanges)
  step("zoom.painted", getMapZoom)
  step("setZoom.painted", setMapZoom, 5)
  step("zoom.set", getMapZoom)
  step("zoomArea.set", getMapZoom, area)
  step("zoomOther.set", getMapZoom, other)
  step("setZoom.small", setMapZoom, 1)
  step("setZoom.nan", setMapZoom, 0/0)
  step("setZoom.noArea", setMapZoom, 4, 9999)
  step("setZoom.other", setMapZoom, 8, other)
  step("zoom.other", getMapZoom)
  step("centerview.far", centerview, far)
  step("zoom.far", getMapZoom)
  settle()
  step("changes.far", areaChanges)
  step("zoom.far.painted", getMapZoom)
  settle()
  step("changes.again", areaChanges)
  step("close", closeMapWidget)
  step("centerview.closed", centerview, near)
  settle()
  step("changes.closed", areaChanges)
  step("zoom.closed", getMapZoom)
  step("setZoom.closed", setMapZoom, 9)
  step("zoomOther.closed", getMapZoom, other)
  step("reopen", openMapWidget)
  step("zoom.reopen", getMapZoom)
  settle()
  step("changes.reopen", areaChanges)
  step("zoom.reopen.painted", getMapZoom)
  -- A paint leaves the area alone while the player stays in the same room, until a centring is asked for
  step("move", setRoomArea, near, other)
  updateMap()
  settle()
  step("changes.moved", areaChanges)
  step("zoom.moved", getMapZoom)
  step("centerview.moved", centerview, near)
  settle()
  step("changes.recentred", areaChanges)
  step("zoom.recentred", getMapZoom)
  step("moveBack", setRoomArea, near, area)
  step("centerview.back", centerview, near)
  settle()
  step("changes.back", areaChanges)
  local path = getMudletHomeDir() .. "/headless map view.dat"
  step("save", saveMap, path)
  step("centerview.far2", centerview, far)
  settle()
  step("changes.far2", areaChanges)
  step("zoom.far2", getMapZoom)
  step("load", loadMap, path)
  step("zoom.load", getMapZoom)
  settle()
  step("changes.load", areaChanges)
  step("zoom.load.painted", getMapZoom)
  local jsonPath = getMudletHomeDir() .. "/headless map view.json"
  step("saveJson", saveJsonMap, jsonPath)
  step("centerview.far3", centerview, far)
  settle()
  step("changes.far3", areaChanges)
  step("loadJson", loadJsonMap, jsonPath)
  step("zoom.loadJson", getMapZoom)
  settle()
  step("changes.loadJson", areaChanges)
  step("zoom.loadJson.painted", getMapZoom)
  -- A map load asks the mapper to show itself, which does not open a closed map widget
  step("centerview.far4", centerview, far)
  settle()
  step("changes.far4", areaChanges)
  step("close.json", closeMapWidget)
  step("loadJson.closed", loadJsonMap, jsonPath)
  settle()
  step("changes.loadJson.closed", areaChanges)
  step("title.loadJson.closed", getMapWindowTitle)
  step("reopen.json", openMapWidget)
  settle()
  step("changes.reopen.json", areaChanges)
  step("delete", deleteRoom, near)
  settle()
  step("changes.delete", areaChanges)
  step("zoom.delete", getMapZoom)
  step("deleteArea", deleteArea, other)
  settle()
  step("changes.deleteArea", areaChanges)
  step("zoom.deleteArea", getMapZoom)
  step("setZoom.deleteArea", function()
    local zoomed, message = setMapZoom(6)
    return zoomed, (tostring(message):gsub(other, "<other>"))
  end)
  -- Left pending as this profile goes, which must not keep the next profile's map from hearing its own
  updateMap()
)lua"),
                                                  answers1);
        QVERIFY2(different1.isEmpty(), qPrintable(different1.join(QChar::LineFeed)));

        // An embedded mapper taking over a hidden map widget starts again, as the GUI's new dlgMapper does
        QStringList answers2{qsl("open=[1]true"),
                             qsl("centerview=[1]true"),
                             qsl("changes.open=[1]other<-2"),
                             qsl("zoom.open=[1]20"),
                             qsl("close=[1]true"),
                             qsl("embedded=[1]true"),
                             qsl("zoom.embedded=[1]20"),
                             qsl("setZoom.embedded=[2]nil:number 0 is not a valid areaID"),
                             qsl("changes.embedded=[1]other<-2"),
                             qsl("zoom.embedded.painted=[1]20"),
                             qsl("setZoom.embedded.painted=[1]true"),
                             qsl("zoomOther.embedded=[1]6"),
                             qsl("centerview.near=[1]true"),
                             qsl("embedded.again=[1]true"),
                             qsl("changes.near=[1]area<other"),
                             qsl("zoom.near=[1]20"),
                             qsl("perspective=[2]nil:the modern 3D map view is not showing"),
                             qsl("setPerspective=[2]nil:the modern 3D map view is not showing"),
                             qsl("hide=[1]true"),
                             qsl("centerview.hidden=[1]true"),
                             qsl("changes.hidden=[1]"),
                             qsl("zoom.hidden=[1]20"),
                             qsl("show=[1]true"),
                             qsl("changes.shown=[1]other<area"),
                             qsl("zoom.shown=[1]6"),
                             qsl("moveToBox=[1]true"),
                             qsl("centerview.box=[1]true"),
                             qsl("changes.box=[1]"),
                             qsl("showBox=[1]true"),
                             qsl("changes.boxShown=[1]area<other"),
                             qsl("zoom.boxShown=[1]20"),
                             qsl("moveHidden=[1]true"),
                             qsl("centerview.moveHidden=[1]true"),
                             qsl("changes.moveHidden=[1]"),
                             qsl("embedded.reshown=[1]true"),
                             qsl("changes.reshown=[1]other<area"),
                             qsl("zoom.reshown=[1]6"),
                             qsl("stay=[1]true"),
                             qsl("centerview.stay=[1]true"),
                             qsl("changes.stay=[1]area<other"),
                             qsl("intoBox=[1]true"),
                             qsl("boxIntoWindow=[1]true"),
                             qsl("centerview.boxHidden=[1]true"),
                             qsl("changes.boxHidden=[1]"),
                             qsl("changes.boxWindowShown=[1]other<area"),
                             qsl("saveJson.far=[1]true"),
                             qsl("centerview.near.json=[1]true"),
                             qsl("changes.near.json=[1]area<other"),
                             qsl("saveJson.near=[1]true"),
                             qsl("hideByMove=[1]true"),
                             qsl("loadJson.hiddenByMove=[1]true"),
                             qsl("changes.loadJson.hiddenByMove=[1]other<area"),
                             qsl("loadJson.windowHidden=[1]true"),
                             qsl("changes.loadJson.windowHidden=[1]"),
                             qsl("changes.windowShownAgain=[1]area<other"),
                             qsl("centerview.far.dat=[1]true"),
                             qsl("changes.far.dat=[1]other<area"),
                             qsl("saveDat=[1]true"),
                             qsl("centerview.near.dat=[1]true"),
                             qsl("changes.near.dat=[1]area<other"),
                             qsl("hideByMove.dat=[1]true"),
                             qsl("loadDat.hiddenByMove=[1]true"),
                             qsl("changes.loadDat.hiddenByMove=[1]other<area"),
                             qsl("hideByMove.failed=[1]true"),
                             qsl("loadMissing.hiddenByMove=[1]false"),
                             qsl("rooms.failed=[2]true:true"),
                             qsl("centerview.again=[1]true"),
                             qsl("painted.loadMissing.hiddenByMove=[1]1")};
#if !defined(INCLUDE_3DMAPPER)
        answers2.removeIf([](const QString& answer) {
            return answer.startsWith(QLatin1String("perspective=")) || answer.startsWith(QLatin1String("setPerspective="));
        });
#endif
        const QStringList different2 = mismatches(qsl(R"lua(
  step("open", openMapWidget)
  step("centerview", centerview, far)
  settle()
  step("changes.open", areaChanges)
  step("zoom.open", getMapZoom)
  step("close", closeMapWidget)
  -- Only once the map widget is closed: a user window docked beside it kept the GUI's map from painting
  openUserWindow("headlessMapViewWindow")
  step("embedded", createMapper, "headlessMapViewWindow", 0, 0, 400, 300)
  step("zoom.embedded", getMapZoom)
  step("setZoom.embedded", setMapZoom, 5)
  settle()
  step("changes.embedded", areaChanges)
  step("zoom.embedded.painted", getMapZoom)
  step("setZoom.embedded.painted", setMapZoom, 6)
  step("zoomOther.embedded", getMapZoom, other)
  step("centerview.near", centerview, near)
  step("embedded.again", createMapper, "headlessMapViewWindow", 0, 0, 400, 300)
  settle()
  step("changes.near", areaChanges)
  step("zoom.near", getMapZoom)
  -- The GUI's mapper shows its 2D map, so its 3D camera cannot move; without the 3D mapper there are no such calls
  if shiftMapPerspective then
    step("perspective", shiftMapPerspective, 10, 20, 30)
    step("setPerspective", setMapPerspective, 500, 10, 20)
  end
  step("hide", hideWindow, "headlessMapViewWindow")
  step("centerview.hidden", centerview, far)
  settle()
  step("changes.hidden", areaChanges)
  step("zoom.hidden", getMapZoom)
  step("show", showWindow, "headlessMapViewWindow")
  settle()
  step("changes.shown", areaChanges)
  step("zoom.shown", getMapZoom)
  createScrollBox("headlessMapViewBox", 0, 0, 400, 300)
  hideWindow("headlessMapViewBox")
  step("moveToBox", setWindow, "headlessMapViewBox", "mapper", 0, 0, true)
  step("centerview.box", centerview, near)
  settle()
  step("changes.box", areaChanges)
  step("showBox", showWindow, "headlessMapViewBox")
  settle()
  step("changes.boxShown", areaChanges)
  step("zoom.boxShown", getMapZoom)
  openUserWindow("headlessMapViewOther")
  step("moveHidden", setWindow, "headlessMapViewOther", "mapper", 0, 0, false)
  step("centerview.moveHidden", centerview, far)
  settle()
  step("changes.moveHidden", areaChanges)
  step("embedded.reshown", createMapper, "headlessMapViewWindow", 0, 0, 400, 300)
  settle()
  step("changes.reshown", areaChanges)
  step("zoom.reshown", getMapZoom)
  step("stay", setWindow, "headlessMapViewOther", "mapper", 0, 0, false)
  step("centerview.stay", centerview, near)
  settle()
  step("changes.stay", areaChanges)
  step("intoBox", setWindow, "headlessMapViewBox", "mapper", 0, 0, true)
  step("boxIntoWindow", setWindow, "headlessMapViewOther", "headlessMapViewBox", 0, 0, true)
  hideWindow("headlessMapViewOther")
  step("centerview.boxHidden", centerview, far)
  settle()
  step("changes.boxHidden", areaChanges)
  showWindow("headlessMapViewOther")
  settle()
  step("changes.boxWindowShown", areaChanges)
  -- A map load asks the mapper to show itself: one that setWindow() hid shows, one in a hidden window does not
  local farJson = getMudletHomeDir() .. "/headless map view far.json"
  local nearJson = getMudletHomeDir() .. "/headless map view near.json"
  step("saveJson.far", saveJsonMap, farJson)
  step("centerview.near.json", centerview, near)
  settle()
  step("changes.near.json", areaChanges)
  step("saveJson.near", saveJsonMap, nearJson)
  step("hideByMove", setWindow, "headlessMapViewWindow", "mapper", 0, 0, false)
  step("loadJson.hiddenByMove", loadJsonMap, farJson)
  settle()
  step("changes.loadJson.hiddenByMove", areaChanges)
  hideWindow("headlessMapViewWindow")
  step("loadJson.windowHidden", loadJsonMap, nearJson)
  settle()
  step("changes.loadJson.windowHidden", areaChanges)
  showWindow("headlessMapViewWindow")
  settle()
  step("changes.windowShownAgain", areaChanges)
  -- So does a binary map load, and one that fails, which keeps the map it had
  local datPath = getMudletHomeDir() .. "/headless map view far.dat"
  step("centerview.far.dat", centerview, far)
  settle()
  step("changes.far.dat", areaChanges)
  step("saveDat", saveMap, datPath)
  step("centerview.near.dat", centerview, near)
  settle()
  step("changes.near.dat", areaChanges)
  step("hideByMove.dat", setWindow, "headlessMapViewOther", "mapper", 0, 0, false)
  step("loadDat.hiddenByMove", loadMap, datPath)
  settle()
  step("changes.loadDat.hiddenByMove", areaChanges)
  step("hideByMove.failed", setWindow, "headlessMapViewWindow", "mapper", 0, 0, false)
  step("loadMissing.hiddenByMove", loadMap, getMudletHomeDir() .. "/no such map.dat")
  step("rooms.failed", function() return roomExists(near), roomExists(far) end)
  local again = room(addAreaName("Headless Map View Again"))
  step("centerview.again", centerview, again)
  settle()
  step("painted.loadMissing.hiddenByMove", function()
    local painted = #changes
    changes = {}
    return painted
  end)
)lua"),
                                                  answers2);
        QVERIFY2(different2.isEmpty(), qPrintable(different2.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the map view created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones,
    // which a spec cannot wait for: a text edit in a deleted scroll box goes with it
    void test_headlessTextEditContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Text-Edit");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessTextEdit = "not run"
local deletedEvents = {}
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      -- Bytes a line of answers cannot hold as they are, by number
      values[i] = tostring((select(i, ...))):gsub("[%c\128-\255]", function(byte)
        return "\\" .. byte:byte()
      end)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- By name, as the C++ registerAnonymousEventHandler() takes no function
  function headlessTextEditDeleted(_, name)
    deletedEvents[#deletedEvents + 1] = name
  end
  registerAnonymousEventHandler("sysTextEditDeleted", "headlessTextEditDeleted")
  local function events(label)
    answers[#answers + 1] = label .. ":events=" .. table.concat(deletedEvents, ",")
    for i = #deletedEvents, 1, -1 do
      deletedEvents[i] = nil
    end
  end
  step("create", createTextEdit, "teMain", 10, 20, 200, 100)
  look("created", "teMain")
  step("createAgain", createTextEdit, "teMain", 0, 0, 10, 10)
  step("createEmpty", createTextEdit, "", 0, 0, 10, 10)
  step("createInMissing", createTextEdit, "teNoSuch", "teLost", 0, 0, 10, 10)
  look("lost", "teLost")
  step("createInMain", createTextEdit, "main", "teInMain", 1, 2, 30, 40)
  look("inMain", "teInMain")
  step("createInEmpty", createTextEdit, "", "teInEmpty", 1, 2, 30, 40)
  look("inEmpty", "teInEmpty")
  step("createNegative", createTextEdit, "teNegative", 3, 4, -5, -6)
  look("negative", "teNegative")
  step("createBadName", createTextEdit, {}, 0, 0, 10, 10)
  step("createBadX", createTextEdit, "teBad", "x", 0, 10, 10)
  look("bad", "teBad")

  step("getNew", getTextEditText, "teMain")
  step("set", setTextEditText, "teMain", "hello")
  step("getSet", getTextEditText, "teMain")
  step("setOdd", setTextEditText, "teMain", "a\tb\r\nc\rd\194\160e\226\128\168f\226\128\169g\0h")
  step("getOdd", getTextEditText, "teMain")
  step("setNumber", setTextEditText, "teMain", 42)
  step("getNumber", getTextEditText, "teMain")
  step("clear", clearTextEdit, "teMain")
  step("getCleared", getTextEditText, "teMain")
  step("readOnly", setTextEditReadOnly, "teMain", true)
  step("setReadOnly", setTextEditText, "teMain", "read only")
  step("getReadOnly", getTextEditText, "teMain")
  step("writable", setTextEditReadOnly, "teMain", false)
  step("placeholder", setTextEditPlaceholder, "teMain", "type here")
  step("sheet", setTextEditStyleSheet, "teMain", "color: red; font-size: 20pt")
  step("font", setTextEditFont, "teMain", "Bitstream Vera Sans Mono")
  step("fontSize", setTextEditFontSize, "teMain", 15)
  step("fontSizeNegative", setTextEditFontSize, "teMain", -3)
  step("tab", setTextEditTabMovesFocus, "teMain", true)
  step("getOther", getTextEditText, "teInMain")
  look("afterSetters", "teMain")
  for _, f in ipairs({"getTextEditText", "clearTextEdit"}) do
    step("missing." .. f, _G[f], "teNoSuch")
  end
  for _, f in ipairs({"setTextEditText", "setTextEditPlaceholder", "setTextEditStyleSheet", "setTextEditFont"}) do
    step("missing." .. f, _G[f], "teNoSuch", "x")
  end
  step("missing.setTextEditFontSize", setTextEditFontSize, "teNoSuch", 10)
  step("missing.setTextEditReadOnly", setTextEditReadOnly, "teNoSuch", true)
  step("missing.setTextEditTabMovesFocus", setTextEditTabMovesFocus, "teNoSuch", true)
  step("setEmptyName", setTextEditText, "", "x")
  step("setBadName", setTextEditText, {}, "x")
  step("setBadText", setTextEditText, "teMain", {})

  step("raise", raiseWindow, "teMain")
  step("lower", lowerWindow, "teMain")
  step("hide", hideWindow, "teMain")
  look("hidden", "teMain")
  step("show", showWindow, "teMain")
  look("shown", "teMain")
  step("move", moveWindow, "teMain", 5, 6)
  step("resize", resizeWindow, "teMain", 70, 25)
  look("moved", "teMain")
  step("resizeNegative", resizeWindow, "teMain", -1, -2)
  look("resizedNegative", "teMain")
  step("resizeBack", resizeWindow, "teMain", 70, 25)

  assert(openUserWindow("teWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("createInWindow", createTextEdit, "teWindow", "teWin", 1, 1, 50, 20)
  look("inWindow", "teWin")
  hideWindow("teWindow")
  look("inHiddenWindow", "teWin")
  showWindow("teWindow")
  look("inShownWindow", "teWin")
  assert(createScrollBox("teWindow", "teBox", 0, 30, 100, 50))
  step("createInBox", createTextEdit, "teBox", "teBoxed", 2, 2, 40, 20)
  look("inBox", "teBoxed")
  hideWindow("teBox")
  look("inHiddenBox", "teBoxed")
  step("createInHiddenBox", createTextEdit, "teBox", "teLate", 2, 25, 40, 20)
  look("madeInHiddenBox", "teLate")
  showWindow("teBox")
  step("hideBoxed", hideWindow, "teBoxed")
  hideWindow("teBox")
  showWindow("teBox")
  look("boxedStaysHidden", "teBoxed")
  showWindow("teBoxed")
  step("textInBox", setTextEditText, "teBoxed", "boxed")
  step("getInBox", getTextEditText, "teBoxed")

  step("intoBox", setWindow, "teBox", "teMain", 3, 3, true)
  look("movedIntoBox", "teMain")
  hideWindow("teWindow")
  look("movedInHiddenWindow", "teMain")
  showWindow("teWindow")
  step("intoMainHidden", setWindow, "main", "teMain", 4, 4, false)
  look("movedToMainHidden", "teMain")
  step("showMoved", showWindow, "teMain")
  step("intoWindow", setWindow, "teWindow", "teMain", 9, 9, true)
  look("movedIntoWindow", "teMain")
  step("textAfterMove", getTextEditText, "teMain")
  step("intoMain", setWindow, "main", "teMain", 10, 20, true)
  -- What a scroll box holds moves with it
  assert(createScrollBox("teWindow", "teMover", 0, 0, 100, 50))
  step("createInMover", createTextEdit, "teMover", "teRider", 1, 1, 40, 20)
  step("moverIntoMain", setWindow, "main", "teMover", 0, 0, true)
  hideWindow("teWindow")
  look("riderAfterMove", "teRider")
  showWindow("teWindow")
  deleteScrollBox("teMover")
  -- A scroll box takes a text edit before a user window of the same name
  assert(openUserWindow("teTwin", false, false, "f") == true, "openUserWindow did not answer true")
  assert(createScrollBox("teTwin", 0, 0, 100, 50))
  step("createInTwin", createTextEdit, "teTwin", "teInTwin", 1, 1, 40, 20)
  hideWindow("teTwin")
  look("twinWindowHidden", "teInTwin")
  showWindow("teTwin")

  -- A command line of the same name is what the name's geometry and visibility are read from
  step("lineFirst", createCommandLine, "teShared", 1, 1, 10, 10)
  step("editAfterLine", createTextEdit, "teShared", 2, 2, 20, 20)
  look("lineThenEdit", "teShared")
  step("sharedEditText", setTextEditText, "teShared", "edit")
  step("sharedLineText", printCmdLine, "teShared", "line")
  step("sharedGetEdit", getTextEditText, "teShared")
  step("sharedGetLine", getCmdLine, "teShared")
  hideWindow("teShared")
  moveWindow("teShared", 7, 8)
  look("sharedHiddenMoved", "teShared")
  hideWindow("teWindow")
  showWindow("teWindow")
  look("sharedAfterReport", "teShared")
  step("deleteSharedLine", deleteCommandLine, "teShared")
  look("sharedLineDeleted", "teShared")
  step("lineAfterEdit", createCommandLine, "teShared", 3, 3, 30, 30)
  look("editThenLine", "teShared")
  deleteCommandLine("teShared")
  look("lineGoneAgain", "teShared")
  deleteTextEdit("teShared")
  -- And a scroll box before either
  assert(createScrollBox("teTriple", 1, 2, 100, 100))
  assert(createCommandLine("teTriple", 3, 3, 30, 30))
  step("editAfterBoxAndLine", createTextEdit, "teTriple", 5, 5, 50, 50)
  hideWindow("teTriple")
  deleteCommandLine("teTriple")
  look("tripleLineDeleted", "teTriple")
  deleteScrollBox("teTriple")
  look("tripleBoxDeleted", "teTriple")
  deleteTextEdit("teTriple")
  assert(createScrollBox("teBoxShared", 1, 2, 100, 100))
  hideWindow("teBoxShared")
  step("editAfterBox", createTextEdit, "teBoxShared", 5, 6, 40, 20)
  look("boxThenEdit", "teBoxShared")
  step("resizeBoxShared", resizeWindow, "teBoxShared", 60, 70)
  step("deleteBoxShared", deleteScrollBox, "teBoxShared")
  look("sharedBoxDeleted", "teBoxShared")
  step("editBeforeBox", createTextEdit, "teBoxLater", 5, 6, 40, 20)
  moveWindow("teBoxLater", 9, 9)
  createScrollBox("teBoxLater", 1, 2, 100, 100)
  deleteScrollBox("teBoxLater")
  look("movedBeforeBox", "teBoxLater")

  step("delete", deleteTextEdit, "teInMain")
  events("delete")
  look("deleted", "teInMain")
  step("getDeleted", getTextEditText, "teInMain")
  step("setDeleted", setTextEditText, "teInMain", "x")
  step("fontDeleted", setTextEditFontSize, "teInMain", 10)
  step("raiseDeleted", raiseWindow, "teInMain")
  step("deleteAgain", deleteTextEdit, "teInMain")
  step("deleteEmpty", deleteTextEdit, "")
  step("deleteMissing", deleteTextEdit, "teNoSuch")
  step("deleteBadName", deleteTextEdit, {})
  events("failedDeletes")
  step("recreate", createTextEdit, "teInMain", 1, 2, 30, 40)
  step("getRecreated", getTextEditText, "teInMain")
  step("deleteRecreated", deleteTextEdit, "teInMain")
  events("recreated")

  assert(createTextEdit("teBoxedLine", 6, 7, 30, 30))
  step("lineInBoxForEdit", createCommandLine, "teBox", "teBoxedLine", 1, 1, 20, 20)
  look("boxedLineOverEdit", "teBoxedLine")
  step("deleteBox", deleteScrollBox, "teBox")
  events("box")
  look("boxedAtOnce", "teBoxed")
  step("getBoxedAtOnce", getTextEditText, "teBoxed")
  -- A box in a box gives up its name when the outer one goes
  assert(createScrollBox("teOuter", 0, 0, 100, 100))
  assert(createScrollBox("teOuter", "teNested", 0, 0, 50, 50))
  hideWindow("teNested")
  step("editNamedForNested", createTextEdit, "teNested", 11, 12, 40, 20)
  deleteScrollBox("teOuter")
  closeUserWindow("teWindow")
  closeUserWindow("teTwin")
  deleteScrollBox("teTwin")
  -- What a deleted user window holds goes with it, once its deferred delete has run
  assert(openUserWindow("teDoomed", false, false, "f") == true, "openUserWindow did not answer true")
  assert(createTextEdit("teDoomed", "teDoomedEdit", 0, 0, 40, 20))
  assert(createTextEdit("teDoomedLine", 8, 9, 30, 30))
  assert(createCommandLine("teDoomed", "teDoomedLine", 1, 1, 20, 20))
  deleteMiniConsole("teDoomed")
  for _, name in ipairs({"teMain", "teNegative", "teInEmpty", "teBoxShared", "teBoxLater"}) do
    deleteTextEdit(name)
  end
  events("end")
  headlessTextEditAnswers = table.concat(answers, "\n")
end)
headlessTextEdit = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessTextEditAfter()
  local answers = {}
  for _, name in ipairs({"teMain", "teWin", "teBoxed", "teLate", "teRider", "teBox", "teWindow", "teInTwin", "teDoomedEdit", "teDoomed"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  for _, name in ipairs({"teNested", "teBoxedLine", "teDoomedLine"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name)) .. ":" .. table.concat({getWindowGeometry(name)}, ",") .. ":" .. tostring(windowVisible(name))
  end
  answers[#answers + 1] = "events:" .. table.concat(deletedEvents, ",")
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessTextEdit"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("created:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("createAgain=[2]false:couldn't create text edit"),
                                  qsl("createEmpty=[2]false:a text edit cannot have an empty string as its name"),
                                  qsl("createInMissing=[2]false:window 'teNoSuch' not found"),
                                  qsl("lost:type=[2]nil:'teLost' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"teLost\" not "
                                      "found,visible=[2]nil:window \"teLost\" not found"),
                                  qsl("createInMain=[1]true"),
                                  qsl("inMain:type=[1]textedit,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createInEmpty=[1]true"),
                                  qsl("inEmpty:type=[1]textedit,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createNegative=[1]true"),
                                  qsl("negative:type=[1]textedit,geometry=[4]3:4:0:0,visible=[1]true"),
                                  qsl("createBadName=raised:createTextEdit: bad argument #1 type (text edit name as string expected, got table!)"),
                                  qsl("createBadX=raised:createTextEdit: bad argument #2 type (text edit x-coordinate as number expected, got string!)"),
                                  qsl("bad:type=[2]nil:'teBad' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"teBad\" not "
                                      "found,visible=[2]nil:window \"teBad\" not found"),
                                  qsl("getNew=[1]"),
                                  qsl("set=[1]true"),
                                  qsl("getSet=[1]hello"),
                                  qsl("setOdd=[1]true"),
                                  qsl("getOdd=[1]a\\9b\\10c\\10d e\\10f\\10g"),
                                  qsl("setNumber=[1]true"),
                                  qsl("getNumber=[1]42"),
                                  qsl("clear=[1]true"),
                                  qsl("getCleared=[1]"),
                                  qsl("readOnly=[1]true"),
                                  qsl("setReadOnly=[1]true"),
                                  qsl("getReadOnly=[1]read only"),
                                  qsl("writable=[1]true"),
                                  qsl("placeholder=[1]true"),
                                  qsl("sheet=[1]true"),
                                  qsl("font=[1]true"),
                                  qsl("fontSize=[1]true"),
                                  qsl("fontSizeNegative=[2]nil:size cannot be 0 or negative"),
                                  qsl("tab=[1]true"),
                                  qsl("getOther=[1]"),
                                  qsl("afterSetters:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("missing.getTextEditText=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.clearTextEdit=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditText=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditPlaceholder=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditStyleSheet=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditFont=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditFontSize=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditReadOnly=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditTabMovesFocus=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("setEmptyName=[2]nil:text edit name '' not found"),
                                  qsl("setBadName=raised:setTextEditText: bad argument #1 type (text edit name as string expected, got table!)"),
                                  qsl("setBadText=raised:setTextEditText: bad argument #2 type (text as string expected, got table!)"),
                                  qsl("raise=[1]true"),
                                  qsl("lower=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("hidden:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]false"),
                                  qsl("show=[1]true"),
                                  qsl("shown:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("move=[1]true"),
                                  qsl("resize=[1]true"),
                                  qsl("moved:type=[1]textedit,geometry=[4]5:6:70:25,visible=[1]true"),
                                  qsl("resizeNegative=[1]true"),
                                  qsl("resizedNegative:type=[1]textedit,geometry=[4]5:6:0:0,visible=[1]true"),
                                  qsl("resizeBack=[1]true"),
                                  qsl("createInWindow=[1]true"),
                                  qsl("inWindow:type=[1]textedit,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("inHiddenWindow:type=[1]textedit,geometry=[4]1:1:50:20,visible=[1]false"),
                                  qsl("inShownWindow:type=[1]textedit,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("createInBox=[1]true"),
                                  qsl("inBox:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("inHiddenBox:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("createInHiddenBox=[1]true"),
                                  qsl("madeInHiddenBox:type=[1]textedit,geometry=[4]2:25:40:20,visible=[1]false"),
                                  qsl("hideBoxed=[1]true"),
                                  qsl("boxedStaysHidden:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("textInBox=[1]true"),
                                  qsl("getInBox=[1]boxed"),
                                  qsl("intoBox=[1]true"),
                                  qsl("movedIntoBox:type=[1]textedit,geometry=[4]3:3:70:25,visible=[1]true"),
                                  qsl("movedInHiddenWindow:type=[1]textedit,geometry=[4]3:3:70:25,visible=[1]false"),
                                  qsl("intoMainHidden=[1]true"),
                                  qsl("movedToMainHidden:type=[1]textedit,geometry=[4]4:4:70:25,visible=[1]false"),
                                  qsl("showMoved=[1]true"),
                                  qsl("intoWindow=[1]true"),
                                  qsl("movedIntoWindow:type=[1]textedit,geometry=[4]9:9:70:25,visible=[1]true"),
                                  qsl("textAfterMove=[1]read only"),
                                  qsl("intoMain=[1]true"),
                                  qsl("createInMover=[1]true"),
                                  qsl("moverIntoMain=[1]true"),
                                  qsl("riderAfterMove:type=[1]textedit,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("createInTwin=[1]true"),
                                  qsl("twinWindowHidden:type=[1]textedit,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("lineFirst=[1]true"),
                                  qsl("editAfterLine=[1]true"),
                                  qsl("lineThenEdit:type=[1]commandline,geometry=[4]1:1:10:10,visible=[1]true"),
                                  qsl("sharedEditText=[1]true"),
                                  qsl("sharedLineText=[0]"),
                                  qsl("sharedGetEdit=[1]edit"),
                                  qsl("sharedGetLine=[1]line"),
                                  qsl("sharedHiddenMoved:type=[1]commandline,geometry=[4]7:8:10:10,visible=[1]false"),
                                  qsl("sharedAfterReport:type=[1]commandline,geometry=[4]7:8:10:10,visible=[1]false"),
                                  qsl("deleteSharedLine=[1]true"),
                                  qsl("sharedLineDeleted:type=[1]textedit,geometry=[4]2:2:20:20,visible=[1]true"),
                                  qsl("lineAfterEdit=[1]true"),
                                  qsl("editThenLine:type=[1]commandline,geometry=[4]3:3:30:30,visible=[1]true"),
                                  qsl("lineGoneAgain:type=[1]textedit,geometry=[4]2:2:20:20,visible=[1]true"),
                                  qsl("editAfterBoxAndLine=[1]true"),
                                  qsl("tripleLineDeleted:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("tripleBoxDeleted:type=[1]textedit,geometry=[4]5:5:50:50,visible=[1]true"),
                                  qsl("editAfterBox=[1]true"),
                                  qsl("boxThenEdit:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("resizeBoxShared=[1]true"),
                                  qsl("deleteBoxShared=[1]true"),
                                  qsl("sharedBoxDeleted:type=[1]textedit,geometry=[4]5:6:40:20,visible=[1]true"),
                                  qsl("editBeforeBox=[1]true"),
                                  qsl("movedBeforeBox:type=[1]textedit,geometry=[4]9:9:40:20,visible=[1]true"),
                                  qsl("delete=[1]true"),
                                  qsl("delete:events=teShared,teTriple,teInMain"),
                                  qsl("deleted:type=[2]nil:'teInMain' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"teInMain\" not "
                                      "found,visible=[2]nil:window \"teInMain\" not found"),
                                  qsl("getDeleted=[2]nil:text edit name 'teInMain' not found"),
                                  qsl("setDeleted=[2]nil:text edit name 'teInMain' not found"),
                                  qsl("fontDeleted=[2]nil:text edit name 'teInMain' not found"),
                                  qsl("raiseDeleted=[1]false"),
                                  qsl("deleteAgain=[2]false:text edit name 'teInMain' not found"),
                                  qsl("deleteEmpty=[2]false:a text edit cannot have an empty string as its name"),
                                  qsl("deleteMissing=[2]false:text edit name 'teNoSuch' not found"),
                                  qsl("deleteBadName=raised:deleteTextEdit: bad argument #1 type (text edit name as string expected, got table!)"),
                                  qsl("failedDeletes:events="),
                                  qsl("recreate=[1]true"),
                                  qsl("getRecreated=[1]"),
                                  qsl("deleteRecreated=[1]true"),
                                  qsl("recreated:events=teInMain"),
                                  qsl("lineInBoxForEdit=[1]true"),
                                  qsl("boxedLineOverEdit:type=[1]commandline,geometry=[4]1:1:20:20,visible=[1]true"),
                                  qsl("deleteBox=[1]true"),
                                  qsl("box:events="),
                                  qsl("boxedAtOnce:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("getBoxedAtOnce=[1]boxed"),
                                  qsl("editNamedForNested=[1]true"),
                                  qsl("end:events=teMain,teNegative,teInEmpty,teBoxShared,teBoxLater")};
        QCOMPARE(luaGlobalString(host, "headlessTextEditAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessTextEditAfterAnswers = headlessTextEditAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{qsl("teMain:nil"),
                                qsl("teWin:textedit"),
                                qsl("teBoxed:nil"),
                                qsl("teLate:nil"),
                                qsl("teRider:nil"),
                                qsl("teBox:nil"),
                                qsl("teWindow:userwindow"),
                                qsl("teInTwin:nil"),
                                qsl("teDoomedEdit:nil"),
                                qsl("teDoomed:nil"),
                                qsl("teNested:textedit:11,12,40,20:true"),
                                qsl("teBoxedLine:textedit:6,7,30,30:true"),
                                qsl("teDoomedLine:textedit:8,9,30,30:true"),
                                qsl("events:")};
        QCOMPARE(luaGlobalString(host, "headlessTextEditAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the text edits created a widget.");
    }

    // What a profile with no view answers the scroll calls, taken from the same Lua run in the GUI self-test
    // profile, which lets each real pane repaint before it is read.
    void test_headlessScrollContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Scroll");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->getLuaInterpreter()->loadGlobal();

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessScroll = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local function answer(f, ...)
    return described(pcall(f, ...))
  end
  local function settled(name)
    return answer(getScroll, name)
  end
  local function park(name, line)
    local called = answer(scrollTo, name, line)
    return called .. settled(name)
  end
  local function fill(name, count)
    for i = 1, count do
      echo(name, "scroll line " .. i .. "\n")
    end
  end

  local missing = "noSuchScrollWindow"
  local answers = {}
  for _, call in ipairs({
    {"getScroll", getScroll, missing},
    {"scrollTo", scrollTo, missing, 1},
    {"scrollToEnd", scrollTo, missing},
    {"scrollUp", scrollUp, missing, 1},
    {"scrollDown", scrollDown, missing, 1},
    {"getScrollBarVisible", getScrollBarVisible, missing},
    {"enableScrollBar", enableScrollBar, missing},
    {"disableScrollBar", disableScrollBar, missing},
    {"enableHorizontalScrollBar", enableHorizontalScrollBar, missing},
    {"disableHorizontalScrollBar", disableHorizontalScrollBar, missing},
    {"enableScrolling", enableScrolling, missing},
    {"disableScrolling", disableScrolling, missing},
    {"scrollingActive", scrollingActive, missing},
  }) do
    answers[#answers + 1] = call[1] .. "=" .. answer(unpack(call, 2))
  end
  scrollMissing = table.concat(answers, ";")

  -- main is cleared first, so every console walks from no lines and reads the same in any profile
  local function walk(name)
    local steps = {}
    local function note(label, value)
      steps[#steps + 1] = label .. "=" .. value
    end
    note("fresh", answer(getScroll, name) .. answer(getScrollBarVisible, name) .. answer(scrollingActive, name))
    fill(name, 60)
    note("filled", settled(name))
    note("bar", answer(enableScrollBar, name) .. answer(getScrollBarVisible, name) .. answer(disableScrollBar, name) .. answer(getScrollBarVisible, name))
    note("hbar", answer(enableHorizontalScrollBar, name) .. answer(disableHorizontalScrollBar, name))
    note("frozen", answer(disableScrolling, name) .. answer(scrollingActive, name) .. park(name, 10))
    note("thawed", answer(enableScrolling, name) .. answer(scrollingActive, name) .. park(name, 60))
    note("to10", park(name, 10))
    note("to20", park(name, 20))
    note("toMinus5", park(name, -5))
    note("up3", answer(scrollUp, name, 3) .. settled(name))
    note("down2", answer(scrollDown, name, 2) .. settled(name))
    fill(name, 5)
    note("fedWhileBack", settled(name) .. answer(getLastLineNumber, name))
    note("frozenWhileBack", answer(disableScrolling, name) .. settled(name))
    enableScrolling(name)
    note("toMinus100", park(name, -100))
    note("toEnd", answer(scrollTo, name) .. settled(name))
    note("to1000", park(name, 1000))
    fill(name, 5)
    note("fedAtEnd", settled(name) .. answer(getLastLineNumber, name))
    park(name, 10)
    clearWindow(name)
    note("cleared", settled(name) .. answer(getLastLineNumber, name))
    fill(name, 30)
    note("fedAfterClear", settled(name) .. answer(getLastLineNumber, name))
    return table.concat(steps, ";")
  end

  fill("main", 3)
  scrollMainAtEnd = tostring(getScroll() == getLastLineNumber("main"))
  clearWindow("main")
  scrollMain = walk("main")
  scrollMainForms = table.concat({park("", 30), park("main", 25), answer(scrollTo, 20) .. settled("main"), answer(scrollTo) .. settled("main"),
    answer(disableScrolling, "") .. answer(scrollingActive, "") .. answer(enableScrolling, "") .. answer(scrollingActive)}, ";")
  assert(createMiniConsole("scrollMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  scrollMini = walk("scrollMini")
  assert(openUserWindow("scrollWindow") == true, "openUserWindow did not answer true")
  scrollWindow = walk("scrollWindow")
  createBuffer("scrollBuffer")
  scrollBuffer = walk("scrollBuffer")
  deleteMiniConsole("scrollMini")
  scrollMiniGone = answer(getScroll, "scrollMini") .. answer(scrollTo, "scrollMini", 1)
end)
headlessScroll = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessScroll"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "scrollMainAtEnd"), qsl("true"));
        const QStringList missingAnswers{qsl("getScroll=[2]nil:msg"),
                                         qsl("scrollTo=[2]nil:msg"),
                                         qsl("scrollToEnd=[2]nil:msg"),
                                         qsl("scrollUp=[2]nil:msg"),
                                         qsl("scrollDown=[2]nil:msg"),
                                         qsl("getScrollBarVisible=[2]nil:msg"),
                                         qsl("enableScrollBar=[2]nil:msg"),
                                         qsl("disableScrollBar=[2]nil:msg"),
                                         qsl("enableHorizontalScrollBar=[2]nil:msg"),
                                         qsl("disableHorizontalScrollBar=[2]nil:msg"),
                                         qsl("enableScrolling=[2]nil:msg"),
                                         qsl("disableScrolling=[2]nil:msg"),
                                         qsl("scrollingActive=[2]nil:msg")};
        QCOMPARE(luaGlobalString(host, "scrollMissing"), missingAnswers.join(QLatin1Char(';')));
        const QStringList mainSteps{qsl("fresh=[1]0[1]true[1]true"),
                                    qsl("filled=[1]60"),
                                    qsl("bar=[0][1]true[0][1]false"),
                                    qsl("hbar=[0][0]"),
                                    qsl("frozen=[2]nil:msg[1]true[0][1]10"),
                                    qsl("thawed=[2]nil:msg[1]true[0][1]60"),
                                    qsl("to10=[0][1]10"),
                                    qsl("to20=[0][1]20"),
                                    qsl("toMinus5=[0][1]55"),
                                    qsl("up3=[0][1]52"),
                                    qsl("down2=[0][1]54"),
                                    qsl("fedWhileBack=[1]54[1]65"),
                                    qsl("frozenWhileBack=[2]nil:msg[1]54"),
                                    qsl("toMinus100=[0][1]0"),
                                    qsl("toEnd=[0][1]65"),
                                    qsl("to1000=[0][1]65"),
                                    qsl("fedAtEnd=[1]70[1]70"),
                                    qsl("cleared=[1]0[1]0"),
                                    qsl("fedAfterClear=[1]30[1]30")};
        QCOMPARE(luaGlobalString(host, "scrollMain"), mainSteps.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "scrollMainForms"), qsl("[0][1]30;[0][1]25;[0][1]20;[0][1]30;[2]nil:msg[1]true[2]nil:msg[1]true"));
        const QStringList subConsoleSteps{qsl("fresh=[1]0[1]false[1]true"),
                                          qsl("filled=[1]60"),
                                          qsl("bar=[0][1]true[0][1]false"),
                                          qsl("hbar=[0][0]"),
                                          qsl("frozen=[1]true[1]false[0][1]60"),
                                          qsl("thawed=[1]true[1]true[0][1]60"),
                                          qsl("to10=[0][1]10"),
                                          qsl("to20=[0][1]20"),
                                          qsl("toMinus5=[0][1]55"),
                                          qsl("up3=[0][1]52"),
                                          qsl("down2=[0][1]54"),
                                          qsl("fedWhileBack=[1]54[1]65"),
                                          qsl("frozenWhileBack=[1]true[1]65"),
                                          qsl("toMinus100=[0][1]0"),
                                          qsl("toEnd=[0][1]65"),
                                          qsl("to1000=[0][1]65"),
                                          qsl("fedAtEnd=[1]70[1]70"),
                                          qsl("cleared=[1]0[1]0"),
                                          qsl("fedAfterClear=[1]30[1]30")};
        QCOMPARE(luaGlobalString(host, "scrollMini"), subConsoleSteps.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "scrollWindow"), subConsoleSteps.join(QLatin1Char(';')));
        // Never painted, so a real buffer never reads back as scrolled
        const QStringList bufferSteps{qsl("fresh=[1]0[1]false[1]true"),
                                      qsl("filled=[1]60"),
                                      qsl("bar=[0][1]true[0][1]false"),
                                      qsl("hbar=[0][0]"),
                                      qsl("frozen=[2]nil:msg[1]true[0][1]60"),
                                      qsl("thawed=[2]nil:msg[1]true[0][1]60"),
                                      qsl("to10=[0][1]60"),
                                      qsl("to20=[0][1]60"),
                                      qsl("toMinus5=[0][1]60"),
                                      qsl("up3=[0][1]60"),
                                      qsl("down2=[0][1]60"),
                                      qsl("fedWhileBack=[1]65[1]65"),
                                      qsl("frozenWhileBack=[2]nil:msg[1]65"),
                                      qsl("toMinus100=[0][1]65"),
                                      qsl("toEnd=[0][1]65"),
                                      qsl("to1000=[0][1]65"),
                                      qsl("fedAtEnd=[1]70[1]70"),
                                      qsl("cleared=[1]0[1]0"),
                                      qsl("fedAfterClear=[1]30[1]30")};
        QCOMPARE(luaGlobalString(host, "scrollBuffer"), bufferSteps.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "scrollMiniGone"), qsl("[2]nil:msg[2]nil:msg"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Scrolling created a widget.");
    }

    // A console scrolled back stays on the same text as the buffer trims the lines above it, as a real view's
    // does once it repaints, so the line it reads back moves up by each trim.
    void test_headlessScrollFollowsTrimmedLines()
    {
        const QString hostname = qsl("Test-Headless-Host-Scroll-Trim");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->getLuaInterpreter()->loadGlobal();

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessTrim = "not run"
local ok, err = pcall(function()
  local function fill(name, count)
    for i = 1, count do
      echo(name, "trim line " .. i .. "\n")
    end
  end
  local function at(name)
    return getScroll(name) .. "/" .. getLastLineNumber(name)
  end
  local function walk(name)
    local steps = {}
    clearWindow(name)
    setConsoleBufferSize(name, 100, 10)
    fill(name, 100)
    steps[#steps + 1] = "filled=" .. at(name)
    scrollTo(name, 50)
    steps[#steps + 1] = "to50=" .. at(name)
    fill(name, 5)
    steps[#steps + 1] = "fed5=" .. at(name)
    fill(name, 10)
    steps[#steps + 1] = "trimmed=" .. at(name)
    fill(name, 10)
    steps[#steps + 1] = "trimmedAgain=" .. at(name)
    scrollTo(name, 5)
    fill(name, 10)
    steps[#steps + 1] = "trimmedPast=" .. at(name)
    scrollDown(name, 2)
    steps[#steps + 1] = "down2=" .. at(name)
    scrollTo(name)
    fill(name, 10)
    steps[#steps + 1] = "trimmedAtEnd=" .. at(name)
    return table.concat(steps, ";")
  end
  trimMain = walk("main")
  assert(createMiniConsole("trimMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  trimMini = walk("trimMini")
  assert(openUserWindow("trimWindow") == true, "openUserWindow did not answer true")
  trimWindow = walk("trimWindow")
end)
headlessTrim = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessTrim"), qsl("ok"));
        // What the same steps read in the GUI self-test profile once each console has repainted
        const QString steps = qsl("filled=90/90;to50=50/90;fed5=50/95;trimmed=40/95;trimmedAgain=30/95;trimmedPast=0/95;down2=2/95;trimmedAtEnd=95/95");
        QCOMPARE(luaGlobalString(host, "trimMain"), steps);
        QCOMPARE(luaGlobalString(host, "trimMini"), steps);
        QCOMPARE(luaGlobalString(host, "trimWindow"), steps);
    }

    // The font and wrap calls answer every kind of window as the GUI self-test profile does, with the pixel
    // sizes those fonts measure; a label takes the font of the console it is made on, and keeps it.
    void test_headlessFontAndWrapContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Font-Wrap");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        const QFont startFont = host->getDisplayFont();

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFontWrap = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local function answer(f, ...)
    return described(pcall(f, ...))
  end
  local startFamily = getFont("main")
  local fonts = getAvailableFonts()
  for _, candidate in ipairs({"DejaVu Sans", "Ubuntu Mono", "Liberation Mono", "Noto Sans Mono"}) do
    if fonts[candidate] then
      fwFamily = candidate
      break
    end
  end
  if not fwFamily then
    for name in pairs(fonts) do
      if name ~= getFont("main") and name ~= "Bitstream Vera Sans Mono" then
        fwFamily = name
        break
      end
    end
  end
  fwGlobal = table.concat({answer(calcFontSize), answer(calcFontSize, true), answer(getFont), answer(getFontSize), answer(getWindowWrap)}, ";")

  assert(createMiniConsole("fwMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  assert(openUserWindow("fwWindow") == true, "openUserWindow did not answer true")
  createBuffer("fwBuffer")
  assert(createLabel("fwLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(createLabel("fwWindow", "fwWindowLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true in a user window")
  local long = string.rep("word ", 14) .. "end"
  local function walk(name)
    local steps = {}
    local function note(label, ...)
      steps[#steps + 1] = label .. "=" .. answer(...)
    end
    note("size", calcFontSize, name)
    note("avg", calcFontSize, name, true)
    note("font", getFont, name)
    note("pt", getFontSize, name)
    note("wrap", getWindowWrap, name)
    note("wrap0", setWindowWrap, name, 0)
    note("wrap30", setWindowWrap, name, 30)
    note("wrapNow", getWindowWrap, name)
    note("indent", setWindowWrapIndent, name, 2)
    note("indentNeg", setWindowWrapIndent, name, -1)
    note("hanging", setWindowWrapHangingIndent, name, 3)
    if getLineCount(name) then
      echo(name, "\n" .. long .. "\n")
      local last = getLastLineNumber(name)
      local lengths = {}
      for i = last - 4, last do
        lengths[#lengths + 1] = #(getLines(name, i, i + 1)[1] or "")
      end
      steps[#steps + 1] = "lengths=" .. table.concat(lengths, ",")
    end
    note("wrapLine", wrapLine, name, 0)
    note("wrapBack", setWindowWrap, name, 100)
    note("pt0", setFontSize, name, 0)
    note("pt13", setFontSize, name, 13)
    note("size13", calcFontSize, name)
    note("avg13", calcFontSize, name, true)
    note("fontEmpty", setFont, name, "")
    note("fontMissing", setFont, name, "No Such Font Family Zz")
    note("fontFamily", setFont, name, fwFamily)
    note("familyFont", getFont, name)
    note("familyPt", getFontSize, name)
    note("familySize", calcFontSize, name)
    note("familyAvg", calcFontSize, name, true)
    return table.concat(steps, ";")
  end
  fwEmpty = walk("")
  fwMain = walk("main")
  fwMiniSteps = walk("fwMini")
  fwWindowSteps = walk("fwWindow")
  fwBufferSteps = walk("fwBuffer")
  fwLabelSteps = walk("fwLabel")
  fwMissing = walk("noSuchFontWindow")

  assert(createLabel("fwLateLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(setFont("main", startFamily) == true, "setFont did not answer true for main")
  assert(createLabel("fwWindow", "fwWindowLateLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true in a user window")
  fwLabelFonts = table.concat({getFont("fwWindowLabel"), getFont("fwLateLabel"), getFont("fwWindowLateLabel")}, ";")
end)
headlessFontWrap = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFontWrap"), qsl("ok"));
        // Main and fwWindow start in one family, so only the sizes show which console a label copied
        const auto labelPointSize = [host](const QString& name) {
            const auto font = host->labelFont(name);
            return font ? font->pointSize() : -1;
        };
        QVERIFY(startFont.pointSize() != 10);
        // The walk's setFontSize(13) reaches a label too
        QCOMPARE(labelPointSize(qsl("fwLabel")), 13);
        QCOMPARE(labelPointSize(qsl("fwWindowLabel")), 10);
        QCOMPARE(labelPointSize(qsl("fwLateLabel")), 13);
        QCOMPARE(labelPointSize(qsl("fwWindowLateLabel")), 13);

        const bool moved = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFontWrap = "not run"
local ok, err = pcall(function()
  assert(createLabel("fwMovedLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(setWindow("fwWindow", "fwMovedLabel", 0, 0, false) == true, "setWindow did not answer true for a label")
  assert(setWindow("main", "fwWindowLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  assert(setWindow("fwWindow", "fwLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  assert(setWindow("main", "fwLateLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  assert(setWindow("fwWindow", "fwWindowLateLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  fwMovedFonts = table.concat({getFont("fwMovedLabel"), getFont("fwWindowLabel"), getFont("fwLabel"), getFont("fwLateLabel"), getFont("fwWindowLateLabel")}, ";")
end)
headlessFontWrap = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(moved, "The Lua chunk moving labels did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFontWrap"), qsl("ok"));

        const auto sized = [](const QFont& font, const int pointSize) {
            TFontAttributes attributes(font);
            attributes.mPointSize = pointSize;
            return attributes.makeFont();
        };
        // A sub-console's font is built as the TConsole constructor builds one, not copied from main's, and the
        // two differ in pitch, kerning and antialiasing, so where the default family is missing they fall back apart
        const auto subConsoleFont = [host](const int pointSize) {
            TFontAttributes attributes(host->fontsAntiAlias());
            attributes.mPointSize = pointSize;
            return attributes.makeFont();
        };
        const auto measured = [](const QFont& font) {
            const QFontMetrics metrics(font);
            return qsl("[2]%1:%2").arg(metrics.horizontalAdvance(QChar('W'))).arg(metrics.height());
        };
        const auto averaged = [](const QFont& font) {
            const QFontMetrics metrics(font);
            return qsl("[2]%1:%2").arg(metrics.averageCharWidth()).arg(metrics.height());
        };
        const auto family = [](const QFont& font) {
            return QFontInfo(font).family();
        };
        const QFont familyFont = host->createFontWithSettings(luaGlobalString(host, "fwFamily"), 13);
        // What the same steps read in the GUI self-test profile, its sizes measured here in the fonts it had
        const auto consoleSteps = [&](const QFont& font, const int wrap) {
            return QStringList{qsl("size=%1").arg(measured(font)),
                               qsl("avg=%1").arg(averaged(font)),
                               qsl("font=[1]%1").arg(family(font)),
                               qsl("pt=[1]%1").arg(font.pointSize()),
                               qsl("wrap=[1]%1").arg(wrap),
                               qsl("wrap0=[2]nil:msg"),
                               qsl("wrap30=[1]true"),
                               qsl("wrapNow=[1]30"),
                               qsl("indent=[1]true"),
                               qsl("indentNeg=[2]nil:msg"),
                               qsl("hanging=[1]true"),
                               qsl("lengths=0,27,28,26,0"),
                               qsl("wrapLine=[1]true"),
                               qsl("wrapBack=[1]true"),
                               qsl("pt0=[2]nil:msg"),
                               qsl("pt13=[1]true"),
                               qsl("size13=%1").arg(measured(sized(font, 13))),
                               qsl("avg13=%1").arg(averaged(sized(font, 13))),
                               qsl("fontEmpty=[2]nil:msg"),
                               qsl("fontMissing=[2]nil:msg"),
                               qsl("fontFamily=[1]true"),
                               qsl("familyFont=[1]%1").arg(family(familyFont)),
                               qsl("familyPt=[1]13"),
                               qsl("familySize=%1").arg(measured(familyFont)),
                               qsl("familyAvg=%1").arg(averaged(familyFont))}
                    .join(QLatin1Char(';'));
        };
        // A label has a font and a point size but no measured size, wrap or text of its own; a missing name has none of them
        const auto otherSteps = [&](const QString& font, const QString& familyAnswer, const int pointSize = 0) {
            const QString refused = qsl("[2]nil:msg");
            const bool label = pointSize > 0;
            return qsl("size=[2]nil:msg;avg=[2]nil:msg;font=%1;pt=%4;wrap=[2]nil:msg;wrap0=[2]nil:msg;wrap30=[2]nil:msg;wrapNow=[2]nil:msg;"
                       "indent=[2]nil:msg;indentNeg=[2]nil:msg;hanging=[2]nil:msg;wrapLine=[2]nil:msg;wrapBack=[2]nil:msg;pt0=[2]nil:msg;pt13=%5;"
                       "size13=[2]nil:msg;avg13=[2]nil:msg;fontEmpty=[2]nil:msg;fontMissing=[2]nil:msg;fontFamily=%2;familyFont=%3;familyPt=%6;"
                       "familySize=[2]nil:msg;familyAvg=[2]nil:msg")
                    .arg(font,
                         familyAnswer.isEmpty() ? refused : qsl("[1]true"),
                         familyAnswer.isEmpty() ? refused : familyAnswer,
                         label ? qsl("[1]%1").arg(pointSize) : refused,
                         label ? qsl("[1]true") : refused,
                         label ? qsl("[1]13") : refused);
        };

        QCOMPARE(luaGlobalString(host, "fwGlobal"), qsl("%1;%2;[1]%3;[1]%4;[1]100").arg(measured(startFont), averaged(startFont), family(startFont)).arg(startFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "fwEmpty"), consoleSteps(startFont, 100));
        QCOMPARE(luaGlobalString(host, "fwMain"), consoleSteps(familyFont, 100));
        // Unlike main, a miniconsole or user window starts with no wrap at all
        QCOMPARE(luaGlobalString(host, "fwMiniSteps"), consoleSteps(subConsoleFont(12), 99999999));
        // Attribute by attribute too, as here the fallbacks of the two constructions measure alike
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("createMiniConsole('fwFreshMini', 0, 0, 300, 200)")));
        const TConsoleModel* pFreshMini = host->consoleModelNamed(qsl("fwFreshMini"));
        QVERIFY(pFreshMini);
        QVERIFY(TFontAttributes(pFreshMini->mUpperPaneFont) == TFontAttributes(subConsoleFont(12)));
        QCOMPARE(luaGlobalString(host, "fwWindowSteps"), consoleSteps(subConsoleFont(10), 99999999));
        QCOMPARE(luaGlobalString(host, "fwBufferSteps"), consoleSteps(subConsoleFont(14), 100));
        QCOMPARE(luaGlobalString(host, "fwLabelSteps"), otherSteps(qsl("[1]%1").arg(family(startFont)), qsl("[1]%1").arg(family(familyFont)), startFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "fwMissing"), otherSteps(qsl("[2]nil:msg"), QString()));
        QCOMPARE(luaGlobalString(host, "fwLabelFonts"), qsl("%1;%2;%2").arg(family(subConsoleFont(10)), family(familyFont)));
        // Moved to another window, one given no font by setFont() takes the application's
        QCOMPARE(luaGlobalString(host, "fwMovedFonts"), qsl("%1;%1;%2;%2;%2").arg(family(QFont()), family(familyFont)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The font and wrap calls created a widget.");
    }

    // A label's sheet sets its font as Qt resolves one, and getLabelSizeHint() measures it, so each
    // step is taken by a real TLabel too, made as TMainConsole makes one, and the answers compared
    void test_headlessLabelStyleSheetFontContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Label-Sheet-Font");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
function lsAnswer(f, ...)
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  return described(pcall(f, ...))
end
lsMissing = lsAnswer(getLabelSizeHint, "lsNoSuchLabel")
createLabel("lsA", 0, 0, 100, 20, 1)
openUserWindow("lsWin")
createLabel("lsWin", "lsB", 0, 0, 100, 20, 1)
createLabel("lsC", 0, 0, 100, 20, 1)
createLabel("lsD", 0, 0, 100, 20, 1)
createLabel("lsE", 0, 0, 100, 20, 1)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "lsMissing"), qsl("[2]nil:msg"));

        // setFont() refuses a family this machine does not have, so each role takes one it does
        const QStringList installed = FontManager::availableFonts();
        QStringList chosen{host->getDisplayFont().family()};
        const auto pick = [&installed, &chosen](const QStringList& candidates) {
            const auto usable = [&chosen](const QString& family) {
                return !chosen.contains(family) && !family.startsWith(QLatin1Char('.')) && !family.contains(QLatin1Char('\''));
            };
            QString family;
            for (const QString& candidate : candidates) {
                if (installed.contains(candidate) && usable(candidate)) {
                    family = candidate;
                    break;
                }
            }
            for (const QString& any : installed) {
                if (!family.isEmpty()) {
                    break;
                }
                if (usable(any)) {
                    family = any;
                }
            }
            chosen << family;
            return family;
        };
        const QString setFamily = pick({qsl("Liberation Serif"), qsl("Times New Roman"), qsl("Georgia")});
        const QString sheetFamily = pick({qsl("DejaVu Sans"), qsl("Helvetica"), qsl("Arial")});
        const QString shorthandFamily = pick({qsl("Liberation Mono"), qsl("Courier New"), qsl("Menlo")});
        QVERIFY2(!setFamily.isEmpty() && !sheetFamily.isEmpty() && !shorthandFamily.isEmpty(), "This machine has too few fonts to tell the steps apart.");

        QStringList mismatches;
        {
            QWidget mainParent;
            mainParent.setFont(host->getDisplayFont());
            mainParent.show();
            QWidget windowParent;
            // The user window's console font, which a label made in it starts with
            windowParent.setFont(host->labelFont(qsl("lsB")).value_or(QFont()));
            windowParent.show();
            const auto makeReference = [host](const QString& name, QWidget* pParent) {
                auto pLabel = new TLabel(host, name, pParent);
                pLabel->setAutoFillBackground(true);
                pLabel->resize(100, 20);
                pLabel->setContentsMargins(0, 0, 0, 0);
                pLabel->show();
                pLabel->setBackgroundColor(QColor(32, 32, 32, 255));
                return pLabel;
            };
            TLabel* pRefA = makeReference(qsl("lsRefA"), &mainParent);
            TLabel* pRefB = makeReference(qsl("lsRefB"), &windowParent);

            const auto compare = [&](const QString& step, const QString& name, const TLabel* pReference) {
                host->getLuaInterpreter()->compileAndExecuteScript(qsl("lsLast = lsAnswer(getLabelSizeHint, '%1')").arg(name));
                const QSize hint = pReference->sizeHint();
                const QString expected = qsl("[2]%1:%2").arg(hint.width()).arg(hint.height());
                if (const QString answer = luaGlobalString(host, "lsLast"); answer != expected) {
                    mismatches << qsl("%1: hint %2, a real label's %3").arg(step, answer, expected);
                }
                const QFont font = host->labelFont(name).value_or(QFont());
                if (font.key() != pReference->font().key()) {
                    mismatches << qsl("%1: font %2, a real label's %3").arg(step, font.toString(), pReference->font().toString());
                }
                if (const QString labelText = host->labelText(name).value_or(QString()); labelText != pReference->text()) {
                    mismatches << qsl("%1: text '%2', a real label's '%3'").arg(step, labelText, pReference->text());
                }
            };
            const auto step = [&](const QString& name, const QString& call, const QString& expected) {
                host->getLuaInterpreter()->compileAndExecuteScript(qsl("lsLast = lsAnswer(function() return %1 end)").arg(call));
                if (const QString answer = luaGlobalString(host, "lsLast"); answer != expected) {
                    mismatches << qsl("%1: %2 answered %3, not %4").arg(name, call, answer, expected);
                }
            };
            const auto sheet = [&](const QString& name, TLabel* pReference, const QString& styleSheet) {
                step(name, qsl("setLabelStyleSheet('%1', [[%2]])").arg(name, styleSheet), qsl("[1]true"));
                pReference->restyle(styleSheet);
                compare(qsl("%1 sheet '%2'").arg(name, styleSheet), name, pReference);
            };
            const auto text = [&](const QString& name, TLabel* pReference, const QString& labelText) {
                step(name, qsl("echo('%1', [[%2]])").arg(name, labelText), qsl("[1]true"));
                pReference->setText(labelText);
                compare(qsl("%1 text '%2'").arg(name, labelText), name, pReference);
            };
            const auto family = [&](const QString& name, TLabel* pReference, const QString& fontFamily) {
                step(name, qsl("setFont('%1', '%2')").arg(name, fontFamily), qsl("[1]true"));
                // TLuaInterpreter::setFont()'s font for a label
                pReference->setFont(host->createFontWithSettings(fontFamily, pReference->font().pointSize()));
                compare(qsl("%1 font %2").arg(name, fontFamily), name, pReference);
            };

            compare(qsl("lsA made"), qsl("lsA"), pRefA);
            // QLabel keeps the hint it gave before any text, through a font change
            family(qsl("lsA"), pRefA, setFamily);
            text(qsl("lsA"), pRefA, qsl("WWWWWWWWWW"));
            sheet(qsl("lsA"), pRefA, qsl("font-family: '%1';").arg(sheetFamily));
            sheet(qsl("lsA"), pRefA, qsl("font-size: 20px;"));
            sheet(qsl("lsA"), pRefA, qsl("font: bold 18px '%1';").arg(shorthandFamily));
            sheet(qsl("lsA"), pRefA, qsl("font-size: 16pt; font-style: italic;"));
            family(qsl("lsA"), pRefA, setFamily);
            step(qsl("lsA"), qsl("setLinkStyle('lsA', 'red', 'blue', false)"), qsl("[1]true"));
            pRefA->setLinkStyle(qsl("red"), qsl("blue"), false);
            sheet(qsl("lsA"), pRefA, qsl("font-size: 16pt; font-style: italic;"));
            sheet(qsl("lsA"), pRefA, qsl("color: red;"));
            text(qsl("lsA"), pRefA, qsl("<b>bold</b> text<br>second line"));
            // The link style replaces an anchor's own inline style, font size and all
            text(qsl("lsA"), pRefA, qsl("<a href=\"x\" style=\"font-size: 40pt\">WWW</a> and <a href=\"y\">y</a>"));
            step(qsl("lsA"), qsl("setLinkStyle('lsA', 'green', 'blue', true)"), qsl("[1]true"));
            pRefA->setLinkStyle(qsl("green"), qsl("blue"), true);
            step(qsl("lsA"), qsl("clearVisitedLinks('lsA')"), qsl("[1]true"));
            pRefA->clearVisitedLinks();
            compare(qsl("lsA visited links cleared"), qsl("lsA"), pRefA);
            sheet(qsl("lsA"), pRefA, QString());
            step(qsl("lsA"), qsl("setBackgroundColor('lsA', 10, 20, 30)"), qsl("[1]true"));
            pRefA->setBackgroundColor(QColor(10, 20, 30));
            compare(qsl("lsA background"), qsl("lsA"), pRefA);

            // A sheet handed back unchanged is skipped, so the cached hint stays, unless link colours were set since
            TLabel* pRefC = makeReference(qsl("lsRefC"), &mainParent);
            compare(qsl("lsC made"), qsl("lsC"), pRefC);
            family(qsl("lsC"), pRefC, setFamily);
            sheet(qsl("lsC"), pRefC, pRefC->styleSheet());
            step(qsl("lsC"), qsl("setLinkStyle('lsC', 'red', 'blue', false)"), qsl("[1]true"));
            pRefC->setLinkStyle(qsl("red"), qsl("blue"), false);
            sheet(qsl("lsC"), pRefC, pRefC->styleSheet());

            // Rules match a label by its class names and its object name, and apply in specificity and then sheet order
            TLabel* pRefD = makeReference(qsl("lsRefD"), &mainParent);
            const QString objectName = qsl("label_%1_lsD").arg(hostname);
            pRefD->setObjectName(objectName);
            text(qsl("lsD"), pRefD, qsl("WWWWWWWWWW"));
            const QStringList ruleSheets{
                    qsl("QLabel { font-size: 20px; }"),
                    qsl("QLabel#%1 { font-family: '%2'; } QLabel { font-size: 18px; font-family: '%3'; }").arg(objectName, sheetFamily, shorthandFamily),
                    qsl("QLabel#lsD { font-size: 40px; } TLabel { font-weight: bold; }"),
                    qsl("* { font-size: 11pt; } QLabel { font-style: italic; } * { font-size: 30px; }"),
                    qsl("QLabel:hover { font-size: 30px; } QLabel:!hover { font-size: 13pt; } QLabel::hover { font-weight: bold; }"),
                    qsl("QLabel:enabled:active { font-size: 21px; } QLabel:!enabled { font-size: 40px; } QLabel:hover:no-such-state { font-style: italic; }"),
                    qsl("QLabel::item { font-size: 40px; } QLabel:no-such-state { font-size: 40px; } QLabel:!focus { font-weight: bold; }"),
                    qsl(".TLabel { font-size: 15px; } .QLabel { font-size: 40px; } QPushButton, QFrame { font-style: italic; }"),
                    qsl("QLabel, TLabel#%1 { font-size: 14px; } QLabel { font-size: 24px; }").arg(objectName),
                    qsl("/* { */ QLabel { font: bold 16px '%1'; } QLabel { color: red; font-size: 19px; }").arg(shorthandFamily),
                    qsl(R"(QLabel { font-family: 'Rock\'n Roll'; font-size: 27px; } QLabel { font-weight: bold; })"),
                    qsl("@media screen { QLabel { font-size: 40px; } } QLabel { font-size: 17px; }"),
                    qsl("QLabel { font-size: 22px; } QLabel { font-size: 40px; "),
                    qsl("QLabel { font-size: 25px; } QLabel"),
                    qsl("QLabel { font-size: 26px; } QLabel { font-weight: bold; { font-size: 40px; } }"),
                    qsl("font-size: 23px;"),
            };
            for (const QString& ruleSheet : ruleSheets) {
                sheet(qsl("lsD"), pRefD, ruleSheet);
            }

            // A sheet's margin, border and padding frame the label, which QLabel then indents by an x
            const QStringList boxSheets{
                    qsl("padding: 4px;"),
                    qsl("padding: 1px 2px; border: 3px solid red;"),
                    qsl("padding: 1px 2px 5px; margin: 2px 3px 4px 6px;"),
                    qsl("padding-left: 7px; padding-top: 3; margin-right: 2px; margin-bottom: 1PX;"),
                    qsl("border-width: 4px;"),
                    qsl("border-width: 4px 2px; border-style: solid none;"),
                    qsl("border: 2px red;"),
                    qsl("border: solid 2px;"),
                    qsl("border: 2px solid; border-style: wavy;"),
                    qsl("border-left: 5px dashed; border-top-width: 3px; border-top-style: solid; border-bottom: 2px none;"),
                    qsl("padding: 2pt; margin: 1em; border: 0.5ex solid;"),
                    qsl("font-size: 20px; padding: 1em 1ex;"),
                    qsl("padding: 10%;"),
                    qsl("margin: 0;"),
                    qsl("margin: -2px;"),
                    qsl("color: red;"),
                    qsl("QLabel { padding: 6px; } QLabel#%1 { padding-left: 1px; } QPushButton { margin: 9px; }").arg(objectName),
                    qsl("QLabel:hover { padding: 9px; } * { border: 1px solid; } QLabel { border-right-width: 4px; }"),
                    // QCss::Parser reads a priority marker and nothing weighs it, so a later rule still wins
                    qsl("padding: 4px !important;"),
                    qsl("border-top-style: solid !important; border-top-width: 3px;"),
                    qsl("QLabel { padding: 6px !important; font-size: 30px !important; } QLabel { padding: 2px ! IMPORTANT; font-size: 18px; }"),
                    qsl("QLabel { padding: 2px; } QLabel { padding: 1px 5px!important; }"),
                    QString(),
            };
            for (const QString& boxSheet : boxSheets) {
                sheet(qsl("lsD"), pRefD, boxSheet);
            }
            TLabel* pRefE = makeReference(qsl("lsRefE"), &mainParent);
            compare(qsl("lsE made"), qsl("lsE"), pRefE);
            for (const QString& boxSheet : {qsl("padding: 3px 4px;"), qsl("border: 2px solid; margin: 1px 2px 3px;"), QString()}) {
                sheet(qsl("lsE"), pRefE, boxSheet);
            }

            text(qsl("lsB"), pRefB, qsl("WWWWWWWWWW"));
            sheet(qsl("lsB"), pRefB, qsl("font-size: 20px;"));
            step(qsl("lsB"), qsl("setWindow('main', 'lsB', 0, 0, true)"), qsl("[1]true"));
            pRefB->setParent(&mainParent);
            pRefB->show();
            compare(qsl("lsB moved to main"), qsl("lsB"), pRefB);
            sheet(qsl("lsB"), pRefB, QString());
            step(qsl("lsB"), qsl("setWindow('lsWin', 'lsB', 0, 0, true)"), qsl("[1]true"));
            pRefB->setParent(&windowParent);
            pRefB->show();
            compare(qsl("lsB moved back unstyled"), qsl("lsB"), pRefB);
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The reference labels outlived the test.");
    }

    // The trim that raises sysBufferShrinkEvent runs inside TBuffer::append(), and echo goes on using the
    // console's model after that returns, so a handler deleting the console must not free it there and then.
    // What a label's and a text edit's sheet limit their size to, taken from the same Lua run in the GUI self-test profile
    void test_headlessSizeLimitsContract()
    {
        const QString hostname = qsl("Test Headless Host Size Limits");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir imageDir;
        QVERIFY(imageDir.isValid());
        const auto writeFile = [&imageDir](const QString& name, const QByteArray& data) {
            QFile file(imageDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        // Three 40x16 frames, a 24x10 image, and an SVG that is 30x12
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("sized.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"30\" height=\"12\"><rect width=\"30\" height=\"12\" fill=\"red\"/></svg>\n")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("lsDir = [==[%1]==]").arg(imageDir.path())));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSizeLimits = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. "=" .. described(pcall(getWindowGeometry, name))
  end
  -- What a label answers to the sheet, its hint, its geometry, and its geometry once resized past the limits each way
  -- Each label's key, sheet, size and text, for the test to make the same label with
  local labelSpecs = {}
  local function label(key, sheet, w, h, text)
    labelSpecs[#labelSpecs + 1] = table.concat({key, sheet, w or 10, h or 10, text or ""}, "\t")
    local name = "ls" .. key
    createLabel(name, 0, 0, w or 10, h or 10, 1)
    if text then
      echo(name, text)
    end
    local parts = {described(pcall(setLabelStyleSheet, name, sheet)), described(pcall(getLabelSizeHint, name)), described(pcall(getWindowGeometry, name))}
    resizeWindow(name, 5, 500)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    resizeWindow(name, 500, 5)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    answers[#answers + 1] = key .. "=" .. table.concat(parts, " ")
  end
  -- A selector escapes the spaces of a profile name
  local profileId = (getProfileName():gsub(" ", "\\ "))
  label("min", "min-width: 50px; min-height: 30px")
  label("max", "max-width: 20px; max-height: 8px", 100, 100)
  label("minText", "min-width: 80px; padding: 2px", 10, 10, "hello")
  label("maxText", "max-width: 8px; padding: 2px", 10, 10, "hello")
  label("fixed", "width: 40px; height: 12px")
  label("widthOverMin", "width: 100px; min-width: 50px; height: 5px; min-height: 20px")
  label("heightOverMin", "height: 30px; min-height: 20px")
  label("heightUnderMax", "height: 30px; max-height: 50px", 100, 100)
  label("negativeMinHeight", "height: 10px; min-height: -5px; padding: 1px")
  label("huge", "min-width: 16777215px; padding: 2px", 1, 1)
  -- QRenderRule::boxSize() adds the box to the size in int, which wraps past INT_MAX, and QCss reads a length above it as INT_MAX
  label("wrapped", "min-width: 2147483640px; padding: 10px", 1, 1)
  label("wrappedMax", "max-width: 2147483640px; padding: 10px", 30, 30)
  label("overInt", "min-width: 3000000000px", 1, 1)
  label("overIntPadded", "min-width: 3000000000px; padding: 10px", 1, 1)
  label("hugePadding", "padding: 2147483640px; min-width: 20px", 1, 1)
  label("underInt", "min-width: -3000000000px; padding: 1px", 1, 1)
  label("hugeEm", "min-width: 200000000em", 1, 1)
  label("widthUnderMax", "width: 30px; max-width: 50px; height: 70px; max-height: 50px", 100, 100)
  label("minOverMax", "min-width: 50px; max-width: 20px; min-height: 9px; max-height: 40px", 30, 30)
  label("em", "font-size: 20px; min-width: 2em; max-height: 1em", 10, 100)
  label("ex", "min-width: 3ex", 1, 1)
  label("number", "min-width: 33; max-height: 44", 1, 100)
  label("percent", "min-width: 40%", 1, 1)
  label("pt", "min-width: 30pt", 1, 1)
  label("negativeMin", "min-width: -10px; padding: 3px", 1, 1)
  label("negativeMax", "max-width: -5px; max-height: -1px", 30, 30)
  label("zeroMax", "max-width: 0px; padding: 3px", 30, 30)
  label("important", "min-width: 45px !important", 1, 1)
  label("framed", "margin: 3px; border: 2px solid red; padding: 1px; min-height: 10px; max-width: 10px", 100, 1)
  label("borderNone", "border: 4px none; min-width: 10px", 1, 1)
  label("nativeBorder", "border: 4px native; padding: 1px", 10, 10, "hi")
  label("nativeStyle", "border-width: 4px; border-style: native", 10, 10, "hi")
  label("nativeStylePadded", "border-width: 4px; border-style: native; padding: 1px", 10, 10, "hi")
  label("nativeMin", "border: 4px native; min-width: 50px", 1, 1, "hi")
  label("nativeZeroMargin", "border: 4px native; margin: 0px", 10, 10, "hi")
  label("nativeRadius", "border: 4px native; border-radius: 0px; padding: 1px", 10, 10, "hi")
  label("nativeRadiusMin", "border: 4px native; border-radius: 3px; min-width: 50px", 1, 1, "hi")
  label("nativeTop", "border-top: 4px native; border-left: 3px solid", 10, 10, "hi")
  label("nativeLeft", "border-left: 3px native; border-top: 2px solid", 10, 10, "hi")
  label("nativeShort", "border: native", 10, 10, "hi")
  label("borderWidth", "border-width: 4px; padding: 1px", 10, 10, "hi")
  label("borderNoStyle", "border: 4px; padding: 1px", 10, 10, "hi")
  label("solidLeft", "border-left: 3px solid", 10, 10, "hi")
  label("leftWidth", "border-left-width: 3px; padding: 1px", 10, 10, "hi")
  label("leftStyled", "border-left-style: solid; border-left-width: 3px", 10, 10, "hi")
  label("topNone", "border-top-style: none; border-left: 3px solid", 10, 10, "hi")
  label("topLeft", "border-top: 2px solid; border-left: 3px solid", 10, 10, "hi")
  label("borderColor", "border-color: red; padding: 1px", 10, 10, "hi")
  label("radiusWidth", "border-radius: 2px; border-width: 4px; padding: 1px", 10, 10, "hi")
  label("cornerRadius", "border-top-left-radius: 2px; border-width: 4px; padding: 1px", 10, 10, "hi")
  label("zeroNative", "border-width: 0px; border-style: native; padding: 1px", 10, 10, "hi")
  label("widthMin", "border-width: 4px; min-width: 50px", 1, 1, "hi")
  label("solidSides", "border-style: solid; border-width: 2px 0px; min-width: 20px", 1, 1, "hi")
  label("borderImage", "border-image: none; border: 4px native; min-width: 50px", 1, 1, "hi")
  label("url", "min-width: 20px; background-image: url(C:/nothing.png)", 1, 1)
  label("quoted", [[min-width: 21px; font-family: "a:b"]], 1, 1)
  label("stray", "min-width: 22px; color", 1, 1)
  label("upper", "MIN-WIDTH: 23PX", 1, 1)
  label("twoValues", "min-width: 24px 30px", 1, 1)
  label("rule", "QLabel { min-width: 60px }", 1, 1)
  label("otherClass", "QPlainTextEdit { min-width: 60px }", 1, 1)
  label("enabled", "QLabel:enabled { min-width: 60px }", 1, 1)
  label("active", "QLabel:active { min-width: 60px }", 1, 1)
  label("inactive", "QLabel:!active { min-width: 60px }", 1, 1)
  label("hover", "QLabel:hover { min-width: 60px }", 1, 1)
  label("notEditable", "QLabel:!editable { min-width: 30px }", 1, 1)
  label("readOnly", "QLabel:read-only { min-width: 30px }", 1, 1)
  label("named", "#label_" .. profileId .. "_lsnamed { min-width: 60px }", 1, 1)
  label("otherNamed", "#label_" .. profileId .. "_lsother { min-width: 60px }", 1, 1)
  label("activePadded", "QLabel:active { padding: 3px; min-width: 40px }", 1, 1, "hi")
  label("inactivePadded", "QLabel:!active { padding: 3px; min-width: 40px }", 1, 1, "hi")
  -- A label made smaller keeps its size once its sheet lets it go, and is held again by a new one
  label("cleared", "min-width: 50px; max-height: 8px", 1, 100)
  step("cleared.clear", setLabelStyleSheet, "lscleared", "")
  look("cleared.after", "lscleared")
  resizeWindow("lscleared", 5, 500)
  look("cleared.free", "lscleared")
  step("cleared.hint", getLabelSizeHint, "lscleared")
  step("cleared.again", setLabelStyleSheet, "lscleared", "max-width: 3px;")
  look("cleared.held", "lscleared")
  step("cleared.color", setBackgroundColor, "lscleared", 255, 0, 0, 255)
  resizeWindow("lscleared", 50, 50)
  look("cleared.colored", "lscleared")
  -- A hidden label is held too
  createLabel("lshidden", 0, 0, 10, 10, 1)
  hideWindow("lshidden")
  step("hidden.sheet", setLabelStyleSheet, "lshidden", "min-width: 40px")
  look("hidden", "lshidden")
  showWindow("lshidden")
  look("hidden.shown", "lshidden")
  -- The hint of an image or a movie is held by the minimum, that of an SVG is not
  local png = lsDir .. "/still.png"
  local svg = lsDir .. "/sized.svg"
  createLabel("lsimage", 0, 0, 10, 10, 1)
  step("image.set", setBackgroundImage, "lsimage", png)
  step("image.hint", getLabelSizeHint, "lsimage")
  step("image.sheet", setLabelStyleSheet, "lsimage", "min-width: 50px; min-height: 3px")
  step("image.hintMin", getLabelSizeHint, "lsimage")
  createLabel("lsmovie", 0, 0, 10, 10, 1)
  step("movie.set", setMovie, "lsmovie", lsDir .. "/anim.gif")
  step("movie.sheet", setLabelStyleSheet, "lsmovie", "min-width: 60px; min-height: 3px")
  step("movie.hint", getLabelSizeHint, "lsmovie")
  createLabel("lssvg", 0, 0, 10, 10, 1)
  step("svg.set", setBackgroundImage, "lssvg", svg)
  step("svg.sheet", setLabelStyleSheet, "lssvg", "min-width: 50px")
  step("svg.hint", getLabelSizeHint, "lssvg")
  look("svg", "lssvg")
  -- The sheet of a text edit limits it the same way
  local function textEdit(key, sheet, w, h)
    local name = "ls" .. key
    createTextEdit(name, 0, 0, w or 10, h or 10)
    local parts = {described(pcall(setTextEditStyleSheet, name, sheet)), described(pcall(getWindowGeometry, name))}
    resizeWindow(name, 5, 500)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    resizeWindow(name, 500, 5)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    answers[#answers + 1] = key .. "=" .. table.concat(parts, " ")
  end
  textEdit("teMin", "min-width: 50px; min-height: 30px")
  textEdit("teMax", "max-width: 20px; max-height: 8px", 100, 100)
  textEdit("tePadded", "min-width: 50px; padding: 2px; border: 3px solid", 1, 1)
  textEdit("teWidth", "width: 100px; min-width: 50px", 1, 1)
  textEdit("teRule", "QPlainTextEdit { min-width: 60px }", 1, 1)
  textEdit("teOwnRule", "TTextBox { min-height: 60px }", 1, 1)
  textEdit("teScrollRule", "QAbstractScrollArea { min-height: 61px }", 1, 1)
  textEdit("teLabelRule", "QLabel { min-width: 60px }", 1, 1)
  textEdit("teNative", "border-style: native; min-width: 50px", 1, 1)
  textEdit("teNativeWidth", "border: 4px native; min-width: 50px", 1, 1)
  textEdit("teEm", "font-size: 20px; min-width: 2em", 1, 1)
  textEdit("teSolid", "border: 2px solid; min-width: 50px", 1, 1)
  textEdit("teNoStyle", "border: 4px; min-width: 50px", 1, 1)
  textEdit("teWidthOnly", "border-width: 3px; min-width: 50px", 1, 1)
  textEdit("teSolidOnly", "border-style: solid; min-width: 50px", 1, 1)
  textEdit("tePadOnly", "padding: 2px; min-width: 50px", 1, 1)
  textEdit("teMargin", "margin: 1px; min-width: 50px", 1, 1)
  textEdit("teTop", "border-top: 2px solid; min-height: 50px; min-width: 50px", 1, 1)
  textEdit("teLeftNative", "border-left: 2px native; min-height: 50px; min-width: 50px", 1, 1)
  textEdit("teRadius", "border-radius: 2px; min-width: 50px", 1, 1)
  textEdit("teCorner", "border-top-left-radius: 2px; min-width: 50px", 1, 1)
  textEdit("teColor", "border-color: red; min-width: 50px", 1, 1)
  textEdit("teNone", "border: none; min-width: 50px", 1, 1)
  textEdit("teColorOnly", "color: red; min-width: 50px", 1, 1)
  textEdit("teEditable", "QPlainTextEdit:editable { min-width: 60px }", 1, 1)
  textEdit("teNotReadOnly", "QPlainTextEdit:!read-only { min-width: 60px }", 1, 1)
  textEdit("teReadOnlyRule", "QPlainTextEdit:read-only { min-width: 60px }", 1, 1)
  textEdit("teActive", "QPlainTextEdit:active { min-width: 60px }", 1, 1)
  textEdit("teInactive", "QPlainTextEdit:!active { padding: 2px; min-width: 50px }", 1, 1)
  textEdit("teFocus", "QPlainTextEdit:focus { min-width: 60px }", 1, 1)
  textEdit("teNamed", "#textEdit_" .. profileId .. "_lsteNamed { min-width: 60px }", 1, 1)
  textEdit("teLabelNamed", "#label_" .. profileId .. "_lsteLabelNamed { min-width: 60px }", 1, 1)
  textEdit("teFrame", "QFrame { min-width: 60px }", 1, 1)
  textEdit("teStar", "* { min-width: 60px }", 1, 1)
  textEdit("teRuleLast", "QPlainTextEdit { min-width: 50px } QAbstractScrollArea { min-width: 70px }", 1, 1)
  textEdit("teRuleFirst", "QAbstractScrollArea { min-width: 70px } QPlainTextEdit { min-width: 50px }", 1, 1)
  createTextEdit("lsteReadOnly", 0, 0, 1, 1)
  setTextEditReadOnly("lsteReadOnly", true)
  step("teReadOnly.sheet", setTextEditStyleSheet, "lsteReadOnly", "QPlainTextEdit:read-only { min-width: 60px }")
  look("teReadOnly", "lsteReadOnly")
  for _, key in ipairs({"Editable", "NotReadOnly"}) do
    createTextEdit("lsteReadOnly" .. key, 0, 0, 1, 1)
    setTextEditReadOnly("lsteReadOnly" .. key, true)
    step("teReadOnly" .. key .. ".sheet", setTextEditStyleSheet, "lsteReadOnly" .. key, key == "Editable" and "QPlainTextEdit:editable { min-width: 60px }" or "QPlainTextEdit:!read-only { min-width: 60px }")
    look("teReadOnly" .. key, "lsteReadOnly" .. key)
  end
  -- Being made read-only does not restyle a text edit, but its next sheet sees it
  createTextEdit("lsteLater", 0, 0, 1, 1)
  setTextEditStyleSheet("lsteLater", "QPlainTextEdit:read-only { min-width: 60px }")
  step("teLater.readOnly", setTextEditReadOnly, "lsteLater", true)
  resizeWindow("lsteLater", 5, 5)
  look("teLater.made", "lsteLater")
  setTextEditStyleSheet("lsteLater", "QPlainTextEdit:read-only { min-width: 60px } ")
  look("teLater.restyled", "lsteLater")
  setTextEditReadOnly("lsteLater", false)
  resizeWindow("lsteLater", 5, 5)
  look("teLater.editable", "lsteLater")
  setTextEditStyleSheet("lsteLater", "QPlainTextEdit:read-only { min-width: 60px }")
  resizeWindow("lsteLater", 5, 5)
  look("teLater.editableRestyled", "lsteLater")
  textEdit("teCleared", "min-width: 50px", 1, 1)
  step("teCleared.clear", setTextEditStyleSheet, "lsteCleared", "")
  resizeWindow("lsteCleared", 5, 5)
  look("teCleared.free", "lsteCleared")
  -- A text edit behind a command line of the same name is still held
  createTextEdit("lsteShared", 0, 0, 10, 10)
  createCommandLine("lsteShared", 0, 0, 10, 10)
  step("teShared.sheet", setTextEditStyleSheet, "lsteShared", "min-width: 50px")
  look("teShared", "lsteShared")
  deleteCommandLine("lsteShared")
  look("teShared.alone", "lsteShared")
  resizeWindow("lsteShared", 5, 5)
  look("teShared.resized", "lsteShared")
  -- and one behind a scroll box of the same name leaves the name to the scroll box
  createScrollBox("lsteBoxed", 0, 0, 40, 40)
  createTextEdit("lsteBoxed", 0, 0, 10, 10)
  step("teBoxed.sheet", setTextEditStyleSheet, "lsteBoxed", "min-width: 50px")
  look("teBoxed", "lsteBoxed")
  step("teMissing.sheet", setTextEditStyleSheet, "lsteMissing", "min-width: 50px")
  -- Moving a text edit to another parent restyles it in the state it is in then, as moving its scroll box does not
  local readOnlyRule = "QPlainTextEdit:read-only { min-width: 60px }"
  openUserWindow("lsWindow", false, false, "f")
  createTextEdit("lsteMoved", 0, 0, 1, 1)
  setTextEditStyleSheet("lsteMoved", readOnlyRule)
  setTextEditReadOnly("lsteMoved", true)
  step("teMoved.toSame", setWindow, "main", "lsteMoved", 3, 3, true)
  resizeWindow("lsteMoved", 5, 5)
  look("teMoved.same", "lsteMoved")
  step("teMoved.toWindow", setWindow, "lsWindow", "lsteMoved", 0, 0, true)
  resizeWindow("lsteMoved", 5, 5)
  look("teMoved.window", "lsteMoved")
  setTextEditReadOnly("lsteMoved", false)
  step("teMoved.toMain", setWindow, "main", "lsteMoved", 0, 0, true)
  resizeWindow("lsteMoved", 5, 5)
  look("teMoved.back", "lsteMoved")
  createScrollBox("lsBox", 0, 0, 100, 100)
  createTextEdit("lsBox", "lsteInBox", 0, 0, 1, 1)
  setTextEditStyleSheet("lsteInBox", readOnlyRule)
  setTextEditReadOnly("lsteInBox", true)
  step("teInBox.moveBox", setWindow, "lsWindow", "lsBox", 0, 0, true)
  resizeWindow("lsteInBox", 5, 5)
  look("teInBox.box", "lsteInBox")
  step("teInBox.moveOut", setWindow, "lsWindow", "lsteInBox", 0, 0, true)
  resizeWindow("lsteInBox", 5, 5)
  look("teInBox.out", "lsteInBox")
  for key in ("borderWidth borderNoStyle solidLeft leftWidth leftStyled topNone topLeft borderColor radiusWidth cornerRadius zeroNative widthMin solidSides borderImage url quoted stray upper twoValues notEditable readOnly activePadded inactivePadded named otherNamed min max minText maxText fixed widthOverMin heightOverMin heightUnderMax negativeMinHeight huge wrapped wrappedMax overInt overIntPadded hugePadding underInt hugeEm widthUnderMax minOverMax em ex number percent pt negativeMin negativeMax zeroMax important framed borderNone nativeBorder nativeStyle nativeStylePadded nativeMin nativeZeroMargin nativeRadius nativeRadiusMin nativeTop nativeLeft nativeShort rule otherClass enabled active inactive hover cleared hidden image movie svg"):gmatch("%S+") do
    deleteLabel("ls" .. key)
  end
  for key in ("teMin teMax tePadded teWidth teRule teOwnRule teScrollRule teLabelRule teNative teNativeWidth teEm teReadOnly teCleared teShared teSolid teNoStyle teWidthOnly teSolidOnly tePadOnly teMargin teTop teLeftNative teRadius teCorner teColor teNone teColorOnly teEditable teNotReadOnly teActive teInactive teFocus teNamed teLabelNamed teFrame teStar teRuleLast teRuleFirst teReadOnlyRule teReadOnlyEditable teReadOnlyNotReadOnly teLater teBoxed teMoved teInBox"):gmatch("%S+") do
    deleteTextEdit("ls" .. key)
  end
  deleteScrollBox("lsteBoxed")
  deleteScrollBox("lsBox")
  closeUserWindow("lsWindow")
  headlessSizeLimitsLabels = table.concat(labelSpecs, "\n")
  headlessSizeLimitsAnswers = table.concat(answers, "\n")
end)
headlessSizeLimits = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSizeLimits"), qsl("ok"));
        // QCss measures a text edit's em in the application font at the sheet's size
        QFont sheetFont;
        sheetFont.setPixelSize(20);
        const int twoEm = 2 * QFontMetrics(sheetFont).height();
        QStringList answers{qsl("min=[1]true {hint} [4]0:0:50:30 [4]0:0:50:500 [4]0:0:500:30"),
                            qsl("max=[1]true {hint} [4]0:0:20:8 [4]0:0:5:8 [4]0:0:20:5"),
                            qsl("minText=[1]true {hint} [4]0:0:84:10 [4]0:0:84:500 [4]0:0:500:5"),
                            qsl("maxText=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:12:5"),
                            qsl("fixed=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("widthOverMin=[1]true {hint} [4]0:0:100:20 [4]0:0:100:500 [4]0:0:500:20"),
                            qsl("heightOverMin=[1]true {hint} [4]0:0:10:30 [4]0:0:5:500 [4]0:0:500:30"),
                            qsl("heightUnderMax=[1]true {hint} [4]0:0:100:30 [4]0:0:5:30 [4]0:0:500:5"),
                            qsl("negativeMinHeight=[1]true {hint} [4]0:0:10:12 [4]0:0:5:500 [4]0:0:500:12"),
                            qsl("huge=[1]true {hint} [4]0:0:16777215:1 [4]0:0:16777215:500 [4]0:0:16777215:5"),
                            qsl("wrapped=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("wrappedMax=[1]true {hint} [4]0:0:30:30 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("overInt=[1]true {hint} [4]0:0:16777215:1 [4]0:0:16777215:500 [4]0:0:16777215:5"),
                            qsl("overIntPadded=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("hugePadding=[1]true {hint} [4]0:0:4:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("underInt=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("hugeEm=[1]true {hint} [4]0:0:16777215:1 [4]0:0:16777215:500 [4]0:0:16777215:5"),
                            qsl("widthUnderMax=[1]true {hint} [4]0:0:30:50 [4]0:0:5:50 [4]0:0:30:5"),
                            qsl("minOverMax=[1]true {hint} [4]0:0:50:30 [4]0:0:50:40 [4]0:0:50:9"),
                            qsl("em=[1]true {hint} {made} {tall} {wide}"),
                            qsl("ex=[1]true {hint} {made} {tall} {wide}"),
                            qsl("number=[1]true {hint} [4]0:0:33:44 [4]0:0:33:44 [4]0:0:500:5"),
                            qsl("percent=[1]true {hint} [4]0:0:40:1 [4]0:0:40:500 [4]0:0:500:5"),
                            qsl("pt=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("negativeMin=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("negativeMax=[1]true {hint} [4]0:0:0:30 [4]0:0:0:500 [4]0:0:0:5"),
                            qsl("zeroMax=[1]true {hint} [4]0:0:6:30 [4]0:0:5:500 [4]0:0:6:5"),
                            qsl("important=[1]true {hint} [4]0:0:45:1 [4]0:0:45:500 [4]0:0:500:5"),
                            qsl("framed=[1]true {hint} [4]0:0:22:22 [4]0:0:5:500 [4]0:0:22:22"),
                            qsl("borderNone=[1]true {hint} [4]0:0:10:1 [4]0:0:10:500 [4]0:0:500:5"),
                            qsl("nativeBorder=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeStyle=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeStylePadded=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeMin=[1]true {hint} [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("nativeZeroMargin=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeRadius=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeRadiusMin=[1]true {hint} [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("nativeTop=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeLeft=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeShort=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("borderWidth=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("borderNoStyle=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("solidLeft=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("leftWidth=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("leftStyled=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("topNone=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("topLeft=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("borderColor=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("radiusWidth=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("cornerRadius=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("zeroNative=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("widthMin=[1]true {hint} [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("solidSides=[1]true {hint} [4]0:0:20:1 [4]0:0:20:500 [4]0:0:500:5"),
                            qsl("borderImage=[1]true {hint} [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("url=[1]true {hint} [4]0:0:20:1 [4]0:0:20:500 [4]0:0:500:5"),
                            qsl("quoted=[1]true {hint} [4]0:0:21:1 [4]0:0:21:500 [4]0:0:500:5"),
                            qsl("stray=[1]true {hint} [4]0:0:22:1 [4]0:0:22:500 [4]0:0:500:5"),
                            qsl("upper=[1]true {hint} [4]0:0:23:1 [4]0:0:23:500 [4]0:0:500:5"),
                            qsl("twoValues=[1]true {hint} [4]0:0:24:1 [4]0:0:24:500 [4]0:0:500:5"),
                            qsl("rule=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("otherClass=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("enabled=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("active=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("inactive=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("hover=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("notEditable=[1]true {hint} [4]0:0:30:1 [4]0:0:30:500 [4]0:0:500:5"),
                            qsl("readOnly=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("named=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("otherNamed=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("activePadded=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("inactivePadded=[1]true {hint} [4]0:0:46:1 [4]0:0:46:500 [4]0:0:500:5"),
                            qsl("cleared=[1]true {hint} [4]0:0:50:8 [4]0:0:50:8 [4]0:0:500:5"),
                            qsl("cleared.clear=[1]true"),
                            qsl("cleared.after=[4]0:0:500:5"),
                            qsl("cleared.free=[4]0:0:5:500"),
                            qsl("cleared.hint={hint}"),
                            qsl("cleared.again=[1]true"),
                            qsl("cleared.held=[4]0:0:3:500"),
                            qsl("cleared.color=[1]true"),
                            qsl("cleared.colored=[4]0:0:3:50"),
                            qsl("hidden.sheet=[1]true"),
                            qsl("hidden=[4]0:0:40:10"),
                            qsl("hidden.shown=[4]0:0:40:10"),
                            qsl("image.set=[1]true"),
                            qsl("image.hint=[2]24:10"),
                            qsl("image.sheet=[1]true"),
                            qsl("image.hintMin=[2]50:10"),
                            qsl("movie.set=[1]true"),
                            qsl("movie.sheet=[1]true"),
                            qsl("movie.hint=[2]60:16"),
                            qsl("svg.set=[1]true"),
                            qsl("svg.sheet=[1]true"),
                            qsl("svg.hint=[2]30:12"),
                            qsl("svg=[4]0:0:50:10"),
                            qsl("teMin=[1]true [4]0:0:52:32 [4]0:0:52:500 [4]0:0:500:32"),
                            qsl("teMax=[1]true [4]0:0:22:10 [4]0:0:5:10 [4]0:0:22:5"),
                            qsl("tePadded=[1]true [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("teWidth=[1]true [4]0:0:102:1 [4]0:0:102:500 [4]0:0:500:5"),
                            qsl("teRule=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teOwnRule=[1]true [4]0:0:1:62 [4]0:0:5:500 [4]0:0:500:62"),
                            qsl("teScrollRule=[1]true [4]0:0:1:63 [4]0:0:5:500 [4]0:0:500:63"),
                            qsl("teLabelRule=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teNative=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teNativeWidth=[1]true [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("teEm=[1]true [4]0:0:%1:1 [4]0:0:%1:500 [4]0:0:500:5").arg(twoEm + 2),
                            qsl("teSolid=[1]true [4]0:0:54:1 [4]0:0:54:500 [4]0:0:500:5"),
                            qsl("teNoStyle=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teWidthOnly=[1]true [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                            qsl("teSolidOnly=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("tePadOnly=[1]true [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                            qsl("teMargin=[1]true [4]0:0:54:1 [4]0:0:54:500 [4]0:0:500:5"),
                            qsl("teTop=[1]true [4]0:0:52:53 [4]0:0:52:500 [4]0:0:500:53"),
                            qsl("teLeftNative=[1]true [4]0:0:53:52 [4]0:0:53:500 [4]0:0:500:52"),
                            qsl("teRadius=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teCorner=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teColor=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teNone=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teColorOnly=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teEditable=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teNotReadOnly=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teReadOnlyRule=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teActive=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teInactive=[1]true [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                            qsl("teFocus=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teNamed=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teLabelNamed=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teFrame=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teStar=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teRuleLast=[1]true [4]0:0:72:1 [4]0:0:72:500 [4]0:0:500:5"),
                            qsl("teRuleFirst=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teReadOnly.sheet=[1]true"),
                            qsl("teReadOnly=[4]0:0:62:1"),
                            qsl("teReadOnlyEditable.sheet=[1]true"),
                            qsl("teReadOnlyEditable=[4]0:0:1:1"),
                            qsl("teReadOnlyNotReadOnly.sheet=[1]true"),
                            qsl("teReadOnlyNotReadOnly=[4]0:0:1:1"),
                            qsl("teLater.readOnly=[1]true"),
                            qsl("teLater.made=[4]0:0:5:5"),
                            qsl("teLater.restyled=[4]0:0:62:5"),
                            qsl("teLater.editable=[4]0:0:62:5"),
                            qsl("teLater.editableRestyled=[4]0:0:5:5"),
                            qsl("teCleared=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teCleared.clear=[1]true"),
                            qsl("teCleared.free=[4]0:0:5:5"),
                            qsl("teShared.sheet=[1]true"),
                            qsl("teShared=[4]0:0:10:10"),
                            qsl("teShared.alone=[4]0:0:52:10"),
                            qsl("teShared.resized=[4]0:0:52:5"),
                            qsl("teBoxed.sheet=[1]true"),
                            qsl("teBoxed=[4]0:0:40:40"),
                            qsl("teMissing.sheet=[2]nil:text edit name 'lsteMissing' not found"),
                            qsl("teMoved.toSame=[1]true"),
                            qsl("teMoved.same=[4]3:3:5:5"),
                            qsl("teMoved.toWindow=[1]true"),
                            qsl("teMoved.window=[4]0:0:62:5"),
                            qsl("teMoved.toMain=[1]true"),
                            qsl("teMoved.back=[4]0:0:5:5"),
                            qsl("teInBox.moveBox=[1]true"),
                            qsl("teInBox.box=[4]0:0:5:5"),
                            qsl("teInBox.moveOut=[1]true"),
                            qsl("teInBox.out=[4]0:0:62:5")};
        // A label's hint, and a length in em or ex, follow fonts nothing here pins, so labels made as the GUI makes them
        // in this process give those
        {
            QWidget parent;
            parent.setFont(host->getDisplayFont());
            parent.show();
            // The GUI's main window is active, which a hint's sheet sees
            parent.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&parent));
            const auto hint = [](const QWidget* pWidget) {
                const QSize size = pWidget->sizeHint();
                return qsl("[2]%1:%2").arg(size.width()).arg(size.height());
            };
            const auto geometry = [](const QWidget* pWidget) {
                const QRect rect = pWidget->geometry();
                return qsl("[4]%1:%2:%3:%4").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
            };
            const auto fill = [&answers](const QString& key, const QString& placeholder, const QString& value) {
                for (QString& answer : answers) {
                    if (answer.startsWith(key + QLatin1Char('='))) {
                        answer.replace(placeholder, value);
                    }
                }
            };
            for (const QString& spec : luaGlobalString(host, "headlessSizeLimitsLabels").split(QLatin1Char('\n'))) {
                // Key, sheet, width, height and text, made as TMainConsole::createLabel(), echo() and setLabelStyleSheet() do
                const QStringList fields = spec.split(QLatin1Char('\t'));
                QCOMPARE(fields.size(), 5);
                const QString& key = fields.at(0);
                auto pLabel = new TLabel(host, qsl("ls%1").arg(key), &parent);
                pLabel->setAutoFillBackground(true);
                pLabel->resize(fields.at(2).toInt(), fields.at(3).toInt());
                pLabel->setContentsMargins(0, 0, 0, 0);
                pLabel->show();
                pLabel->setBackgroundColor(QColor(32, 32, 32, 255));
                if (!fields.at(4).isEmpty()) {
                    pLabel->setText(fields.at(4));
                }
                pLabel->restyle(fields.at(1));
                fill(key, qsl("{hint}"), hint(pLabel));
                fill(key, qsl("{made}"), geometry(pLabel));
                pLabel->resize(5, 500);
                fill(key, qsl("{tall}"), geometry(pLabel));
                pLabel->resize(500, 5);
                fill(key, qsl("{wide}"), geometry(pLabel));
                if (key == qsl("cleared")) {
                    pLabel->restyle(QString());
                    pLabel->resize(5, 500);
                    fill(qsl("cleared.hint"), qsl("{hint}"), hint(pLabel));
                }
            }
        }
        QVERIFY2(!answers.join(QString()).contains(QLatin1Char('{')), "A font-dependent answer was left unfilled.");
        const QStringList actual = luaGlobalString(host, "headlessSizeLimitsAnswers").split(QLatin1Char('\n'));
        QStringList mismatches;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                mismatches << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the labels and text edits created a widget.");
    }

    void test_consoleDeletedByItsOwnShrinkEventWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");
        host->registerAnonymousEventHandler(qsl("sysBufferShrinkEvent"), qsl("onHeadlessShrink"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessShrinks = 0
function onHeadlessShrink(_, name)
  if name == "headlessShrinkBuffer" then
    headlessShrinks = headlessShrinks + 1
    deleteMiniConsole("headlessShrinkBuffer")
  elseif name == "headlessShrinkNested" then
    headlessShrinks = headlessShrinks + 1
    deleteMiniConsole("headlessShrinkWindow")
  end
end
local lines = string.rep("shrink line\n", 150)
createBuffer("headlessShrinkBuffer")
setConsoleBufferSize("headlessShrinkBuffer", 100, 10)
headlessShrinkEcho = tostring(echo("headlessShrinkBuffer", lines))
openUserWindow("headlessShrinkWindow")
createMiniConsole("headlessShrinkWindow", "headlessShrinkNested", 0, 0, 50, 50)
setConsoleBufferSize("headlessShrinkNested", 100, 10)
headlessShrinkNestedEcho = tostring(echo("headlessShrinkNested", lines))
headlessShrinkLeft = table.concat({tostring(windowType("headlessShrinkBuffer")), tostring(windowType("headlessShrinkWindow")), tostring(windowType("headlessShrinkNested"))}, ",")
)lua")));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(luaGlobalString(host, "headlessShrinks").toInt() >= 2, "A shrink event never reached the handler, so nothing was deleted mid-echo.");
        QCOMPARE(luaGlobalString(host, "headlessShrinkEcho"), qsl("true"));
        QCOMPARE(luaGlobalString(host, "headlessShrinkNestedEcho"), qsl("true"));
        QCOMPARE(luaGlobalString(host, "headlessShrinkLeft"), qsl("nil,nil,nil"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(createBuffer("headlessShrinkBuffer"); echo("headlessShrinkBuffer", "made again\n"))lua")));
        const TConsoleModel* remade = host->windowRegistry().subConsoleModel(qsl("headlessShrinkBuffer"));
        QVERIFY2(remade, "A buffer of the deleted one's name could not be made again.");
        QVERIFY(remade->buffer.lineBuffer.join(QChar::LineFeed).contains(qsl("made again")));
    }

    // As MXP_spec.lua's checks of the same tags in the GUI: a frame is a miniconsole under its name, and
    // what the game sends between <DEST> and </DEST> goes there and not into the main window.
    void test_mxpFrameTakesDestTextWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFrames = 'not run'
local ok, err = pcall(function()
  setConfig("specialForceMXPProcessorOn", true)
  headlessDestTriggered = "no"
  local id = tempTrigger("MXPDEST", [[headlessDestTriggered = "yes"]])
  feedTriggers([[<FRAME Name="headlessFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  assert(windowType("headlessFrame") == "miniconsole", "the frame has no console")
  feedTriggers([[<DEST headlessFrame>MXPDEST first</DEST>]] .. "\n")
  feedTriggers([[<DEST headlessFrame>MXPDEST second</DEST>]] .. "\n")
  headlessFrameLines = table.concat(getLines("headlessFrame", 0, getLineCount("headlessFrame") + 1), "|")
  feedTriggers([[<DEST headlessFrame EOF>MXPDEST after clear</DEST>]] .. "\n")
  headlessClearedLines = table.concat(getLines("headlessFrame", 0, getLineCount("headlessFrame") + 1), "|")
  killTrigger(id)
  feedTriggers([[<FRAME headlessFrame ACTION="close">]] .. "\n")
  assert(windowType("headlessFrame") == nil, "the closed frame still has a console")
  setConfig("specialForceMXPProcessorOn", false)
end)
headlessFrames = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFrames"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "headlessFrameLines"), qsl("MXPDEST first|MXPDEST second|"));
        QCOMPARE(luaGlobalString(host, "headlessClearedLines"), qsl("MXPDEST after clear|"));
        QVERIFY2(!mainBufferHolds(host, qsl("MXPDEST")), "Text sent to the frame reached the main console.");
        QCOMPARE(luaGlobalString(host, "headlessDestTriggered"), qsl("no"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The frame created a widget.");
    }

    void test_profileConnectsAndLogsInWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        QByteArray received;
        QTcpSocket* client = nullptr;
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            client = server.nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [&]() {
                received.append(client->readAll());
            });
            client->write("Welcome to the headless test server.\r\n");
        });

        QVERIFY2(HostManager::self()->addHost(mConnectingHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mConnectingHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->setLogin(qsl("headlesshero"));
        host->setPass(qsl("headlesssecret"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessGreeting = 'none'; tempTrigger("headless test server", [[headlessGreeting = line]]))lua")));

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());

        QTRY_COMPARE_WITH_TIMEOUT(luaGlobalString(host, "headlessGreeting"), qsl("Welcome to the headless test server."), 10000);
        QVERIFY2(mainBufferHolds(host, qsl("Welcome to the headless test server.")), "The game's line never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("Open connection made")), "The connection's own messages never reached the main console model.");
        // The login and password go out on timers, 2s and then 1s by default
        QTRY_VERIFY_WITH_TIMEOUT(received.contains("headlesshero") && received.contains("headlesssecret"), 10000);

        host->mTelnet.disconnectIt();
        QTRY_COMPARE_WITH_TIMEOUT(host->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState, 10000);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Connecting the profile created a widget.");
    }

    void test_gameReachesDiscordOverGmcpWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        Discord* discord = Discord::self();
        QVERIFY2(discord, "The profile pool has no Discord presence.");

        const auto gmcp = [](const QByteArray& message) {
            return QByteArrayLiteral("\xff\xfa\xc9") + message + QByteArrayLiteral("\xff\xf0");
        };
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        QByteArray received;
        QTcpSocket* client = nullptr;
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            client = server.nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [&]() {
                received.append(client->readAll());
            });
            // IAC WILL GMCP
            client->write(QByteArrayLiteral("\xff\xfb\xc9"));
        });

        const QString hostname = qsl("Test-Headless-Host-Discord");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QCOMPARE(host->mDiscordMode, Host::DiscordShowGameDetails);

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());
        QTRY_VERIFY_WITH_TIMEOUT(received.contains("Core.Supports.Set"), 10000);
        const bool loaded = discord->libraryLoaded();
        if (loaded && qgetenv("MUDLET_TEST_DISCORD_LIBRARY") == "absent") {
            qWarning() << "A Discord library is installed here, so this run checks the answers with one, not without";
        }
        // Offered, as in the GUI, only when the Discord library could be loaded
        QCOMPARE(received.contains("External.Discord 1"), loaded);
        QCOMPARE(received.contains("External.Discord.Hello"), loaded);

        client->write(gmcp("External.Discord.Status {\"details\": \"Headless details\", \"state\": \"Headless state\"}"));
        client->write(gmcp("External.Discord.Info {\"inviteurl\": \"https://discord.gg/headless\", \"applicationid\": \"123456789012345678\"}"));
        QTRY_COMPARE_WITH_TIMEOUT(host->getDiscordInviteURL(), qsl("https://discord.gg/headless"), 10000);
        QCOMPARE(discord->getDetailText(host), qsl("Headless details"));
        QCOMPARE(discord->getStateText(host), qsl("Headless state"));
        QCOMPARE(discord->getApplicationId(host), qsl("123456789012345678"));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local ok, err = pcall(function()
  local function answer(...)
    local results = {...}
    return tostring(results[1]) .. "|" .. tostring(results[2])
  end
  headlessDiscordAnswers = table.concat({answer(getDiscordDetail()), answer(setDiscordState("Lua state")), answer(getDiscordState()), answer(usingMudletsDiscordID())}, ";")
end)
headlessDiscord = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessDiscord"), qsl("ok"));
        const QString unavailable = qsl("nil|Discord API is not available");
        QCOMPARE(luaGlobalString(host, "headlessDiscordAnswers"), loaded ? qsl("Headless details|nil;true|nil;Lua state|nil;false|nil") : QStringList(4, unavailable).join(QChar(';')));

        host->mTelnet.disconnectIt();
        QTRY_COMPARE_WITH_TIMEOUT(host->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState, 10000);

        // As closing its tab does: the data is keyed by Host*, so a profile made later at that address would inherit it
        QString detailAtTeardown = qsl("not torn down");
        // Scoped to this test, should the profile outlive it
        QObject receiver;
        connect(host, &QObject::destroyed, &receiver, [&detailAtTeardown, discord, host]() {
            detailAtTeardown = discord->getDetailText(host);
        });
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(detailAtTeardown, QString());
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Driving Discord over GMCP created a widget.");
    }

    // Its profiles keep their presence in the first pool's Discord, so have to clear it from there
    void test_aSecondPoolLeavesNoDiscordDataBehind()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString deletedHostname = qsl("Test-Headless-Host-Discord-Deleted");
        const QString droppedHostname = qsl("Test-Headless-Host-Discord-Dropped");
        // Declared first, so it outlives the pool should the test return early
        QStringList detailsAtTeardown;
        auto pool = std::make_unique<HostManager>();
        for (const QString& hostname : {deletedHostname, droppedHostname}) {
            QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
            Host* host = pool->getHost(hostname);
            host->processDiscordGMCP(qsl("External.Discord.Status"), qsl("{\"details\": \"Second pool details\"}"));
            QCOMPARE(Discord::self()->getDetailText(host), qsl("Second pool details"));
            connect(host, &QObject::destroyed, this, [&detailsAtTeardown, host]() {
                detailsAtTeardown << Discord::self()->getDetailText(host);
            });
        }

        pool->deleteHost(deletedHostname);
        pool.reset();
        QCOMPARE(detailsAtTeardown, QStringList({QString(), QString()}));
    }

    // A later profile's exit handler can close an earlier one, then let its game reach it again
    void test_aSecondPoolForgetsDiscordDataAfterEveryExitEvent()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const auto gmcp = [](const QByteArray& message) {
            return QByteArrayLiteral("\xff\xfa\xc9") + message + QByteArrayLiteral("\xff\xf0");
        };
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        QByteArray received;
        QTcpSocket* client = nullptr;
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            client = server.nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [&]() {
                received.append(client->readAll());
            });
            // IAC WILL GMCP
            client->write(QByteArrayLiteral("\xff\xfb\xc9"));
        });

        // Declared first, so they outlive the pool should the test return early
        QString detailAtTeardown = qsl("not torn down");
        bool statusRead = false;
        QObject receiver;
        // Named so the connected profile's exit event comes first, before the other's handler gives it more
        const QString connectedHostname = qsl("Test-Headless-Host-Discord-Exit-A");
        const QString pumpingHostname = qsl("Test-Headless-Host-Discord-Exit-B");
        auto pool = std::make_unique<HostManager>();
        QVERIFY2(pool->addHost(connectedHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(pool->addHost(pumpingHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* connected = pool->getHost(connectedHostname);
        Host* pumping = pool->getHost(pumpingHostname);
        QVERIFY2(connected && pumping, "The profiles are not in the pool.");
        // Stands in for an embedder's slot that closes a profile while the exit handlers run
        static HostManager* spPool = nullptr;
        spPool = pool.get();
        lua_register(pumping->getLuaInterpreter()->getLuaGlobalState(), "dropProfile", [](lua_State* L) -> int {
            spPool->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        // saveJsonMap() pumps events while it writes, and needs a mapper to count the map as open, as in the GUI
        const QString mapPath = mConfigDir.filePath(qsl("headless-exit-pump.json"));
        QVERIFY(pumping->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(assert(openMapWidget()) function onHeadlessExitPump() dropProfile([[%1]]) saveJsonMap([[%2]]) end)lua").arg(connectedHostname, mapPath)));
        pumping->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onHeadlessExitPump"));

        connected->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());
        QTRY_VERIFY_WITH_TIMEOUT(received.contains("Core.Supports.Set"), 10000);
        connect(connected, &QObject::destroyed, &receiver, [&detailAtTeardown, connected]() {
            detailAtTeardown = Discord::self()->getDetailText(connected);
        });
        connect(connected, &Host::signal_discordGameChanged, &receiver, [&statusRead]() {
            statusRead = true;
        });
        // Sent now, but only read once something pumps events: the other profile's exit handler
        client->write(gmcp("External.Discord.Status {\"game\": \"Headless Game\", \"details\": \"Sent while closing\"}"));
        QVERIFY(client->flush());
        pool.reset();

        QVERIFY2(QFileInfo::exists(mapPath), "The exit handler that pumps events never ran.");
        QVERIFY2(statusRead, "The game's Discord status was never read while the pool went.");
        QCOMPARE(detailAtTeardown, QString());
    }

    // An exit handler can reopen its own profile and still be given Discord data by the pump it runs
    void test_deleteHostForgetsDiscordDataOfAProfileReopenedUnderItsExitHandler()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Discord-Reopened");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        // Stand in for an embedder's slot, then for the game's data, both run while the handler pumps events
        lua_State* state = host->getLuaInterpreter()->getLuaGlobalState();
        lua_register(state, "reopenProfile", [](lua_State* L) -> int {
            const QString name = QString::fromUtf8(lua_tostring(L, 1));
            HostManager::self()->deleteHost(name);
            lua_pushboolean(L, HostManager::self()->addHost(name, QString(), QString(), QString()));
            return 1;
        });
        lua_register(state, "gameSendsDiscordDetail", [](lua_State* L) -> int {
            getHostFromLua(L).processDiscordGMCP(qsl("External.Discord.Status"), qsl("{\"details\": \"Sent after reopening\"}"));
            return 0;
        });
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onDiscordReopenExit"));
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onDiscordReopenExit() assert(reopenProfile(getProfileName())) gameSendsDiscordDetail() end")),
                 "The Lua chunk did not run.");

        QString detailAtTeardown = qsl("not torn down");
        QObject receiver;
        connect(host, &QObject::destroyed, &receiver, [&detailAtTeardown, host]() {
            detailAtTeardown = Discord::self()->getDetailText(host);
        });
        HostManager::self()->deleteHost(hostname);
        QCOMPARE(detailAtTeardown, QString());
        QVERIFY2(HostManager::self()->getHost(hostname), "The profile reopened under the exit handler is gone.");
        HostManager::self()->deleteHost(hostname);
    }

    // Reopened under deleteHost(), not under the pool's teardown, which is to take no new profiles
    void test_aSecondPoolForgetsDiscordDataOfAProfileReopenedUnderItsExitHandler()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        static HostManager* spPool = nullptr;
        static QString sDetailAtTeardown;
        sDetailAtTeardown = qsl("not torn down");
        const QString hostname = qsl("Test-Headless-Host-Discord-Pool-Reopened");
        auto pool = std::make_unique<HostManager>();
        spPool = pool.get();
        QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = pool->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        // Stands in for an embedder's slot that reopens the profile, and for the new one's game, while the handler pumps events
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "reopenAndFeedProfile", [](lua_State* L) -> int {
            const QString name = QString::fromUtf8(lua_tostring(L, 1));
            spPool->deleteHost(name);
            Host* reopened = spPool->addHost(name, QString(), QString(), QString()) ? spPool->getHost(name) : nullptr;
            if (reopened) {
                reopened->processDiscordGMCP(qsl("External.Discord.Status"), qsl("{\"details\": \"Sent to the reopened profile\"}"));
                QObject::connect(reopened, &QObject::destroyed, [reopened]() {
                    sDetailAtTeardown = Discord::self()->getDetailText(reopened);
                });
            }
            return 0;
        });
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onPoolReopenExit"));
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onPoolReopenExit() reopenAndFeedProfile(getProfileName()) end")), "The Lua chunk did not run.");

        pool->deleteHost(hostname);
        Host* reopened = pool->getHost(hostname);
        QVERIFY2(reopened, "The exit handler did not reopen the profile.");
        QCOMPARE(Discord::self()->getDetailText(reopened), qsl("Sent to the reopened profile"));
        pool.reset();
        QCOMPARE(sDetailAtTeardown, QString());
    }

    // As STT_spec.lua's refusals in the GUI: each is returned and raised as sysSTTError on the calling profile,
    // which alternates, so a refusal sent to whoever called last would land on the other
    void test_speechToTextRefusesAsTheGuiDoesWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QVERIFY2(TSpeechBridge::instance(), "The profile pool has no speech bridge.");

        const QStringList hostnames{qsl("Test-Headless-Host-Speech-Refusals-A"), qsl("Test-Headless-Host-Speech-Refusals-B")};
        const auto closeProfiles = qScopeGuard([&hostnames]() {
            for (const QString& hostname : hostnames) {
                if (HostManager::self()->hostLoaded(hostname)) {
                    HostManager::self()->deleteHost(hostname);
                }
            }
        });
        QList<Host*> hosts;
        for (const QString& hostname : hostnames) {
            QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
            Host* host = HostManager::self()->getHost(hostname);
            QVERIFY2(host, "The profile is not in the pool.");
            host->registerAnonymousEventHandler(qsl("sysSTTError"), qsl("onHeadlessSttError"));
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSttErrors = {} function onHeadlessSttError(_, message) headlessSttErrors[#headlessSttErrors + 1] = message end")));
            hosts << host;
        }
        // Only with no engine is every one of these refused, so only then is every binding's caller checked
        const bool noEngine = !TSpeechBridge::instance()->speechRecognizer() && !speechEngineAvailable();
#if defined(INCLUDE_SPEECH_ENGINES)
        const bool unloadRefused = false;
#else
        // with no library it could have loaded, there is none to unload
        const bool unloadRefused = true;
#endif
        const QList<QPair<QString, bool>> calls{{qsl("stt.init('/nonexistent/headless/model')"), true},
                                                {qsl("stt.start()"), true},
                                                {qsl("stt.toggle()"), true},
                                                {qsl("stt.setSilenceTimeout(1000)"), true},
                                                {qsl("stt.setSensitivity('short')"), true},
                                                {qsl("stt.setVocabulary({'north'})"), true},
                                                {qsl("stt.stop()"), false},
                                                {qsl("stt.cancel()"), false},
                                                {qsl("stt.close()"), false},
                                                {qsl("stt.reloadLibrary()"), true},
                                                {qsl("stt.unloadLibrary()"), unloadRefused},
                                                {qsl("stt.start()"), true},
                                                {qsl("stt.reloadLibrary()"), true}};
        for (int i = 0; i < calls.size(); ++i) {
            Host* caller = hosts.at(i % 2);
            Host* other = hosts.at((i + 1) % 2);
            const QString& call = calls.at(i).first;
            for (Host* host : hosts) {
                QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSttErrors = {}")));
            }
            QVERIFY(caller->getLuaInterpreter()->compileAndExecuteScript(qsl("local ok, why = %1 headlessSttRefusal = (not ok and type(why) == 'string') and why or ''").arg(call)));
            const QString refusal = luaGlobalString(caller, "headlessSttRefusal");
            for (Host* host : hosts) {
                QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSttRaised = table.concat(headlessSttErrors, '\\n')")));
            }
            QVERIFY2(luaGlobalString(other, "headlessSttRaised").isEmpty(), qPrintable(qsl("%1 raised its refusal on the other profile").arg(call)));
            // An engine, if there is one, answers some refusals without raising them and raises faults of its own
            if (noEngine) {
                QVERIFY2(refusal.isEmpty() != calls.at(i).second, qPrintable(qsl("%1 answered \"%2\"").arg(call, refusal)));
                QCOMPARE(luaGlobalString(caller, "headlessSttRaised"), refusal);
            }
        }
    }

    // Another profile's call, made from inside this one's by a handler, must not take this one's refusal
    void test_aRefusalReachesItsCallerPastANestedCallWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        if (speechEngineAvailable()) {
            QSKIP("A speech engine is available here, so stt.reloadLibrary() is not refused");
        }
        const auto restoreFactory = qScopeGuard([]() {
            SpeechRecognizerFactory::setFactoryOverride(nullptr);
        });
        QVERIFY2(installHeadlessStandInEngine(), "The stand-in engine was not installed.");
        const QStringList hostnames{qsl("Test-Headless-Host-Speech-Nested-A"), qsl("Test-Headless-Host-Speech-Nested-B")};
        const auto closeProfiles = qScopeGuard([&hostnames]() {
            for (const QString& hostname : hostnames) {
                if (HostManager::self()->hostLoaded(hostname)) {
                    HostManager::self()->deleteHost(hostname);
                }
            }
        });
        QList<Host*> hosts;
        for (const QString& hostname : hostnames) {
            QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
            Host* host = HostManager::self()->getHost(hostname);
            QVERIFY2(host, "The profile is not in the pool.");
            host->registerAnonymousEventHandler(qsl("sysSTTError"), qsl("onHeadlessSttError"));
            host->registerAnonymousEventHandler(qsl("sysSTTCapabilitiesChanged"), qsl("onHeadlessSttCapabilities"));
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(
                    qsl("headlessSttErrors = 0 function onHeadlessSttError() headlessSttErrors = headlessSttErrors + 1 end function onHeadlessSttCapabilities() end")));
            hosts << host;
        }
        // B reacts to a capability change with a call that makes it the last caller and raises nothing here
        QVERIFY(hosts.at(1)->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSttNested = 0 function onHeadlessSttCapabilities() headlessSttNested = headlessSttNested + 1 stt.close() end")));
        standInEngine()->releaseResources();
        standInEngine()->setBiasing(false);

        QVERIFY(hosts.at(0)->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSttReloaded = tostring(stt.reloadLibrary())")));
        QCOMPARE(luaGlobalString(hosts.at(0), "headlessSttReloaded"), qsl("false"));
        QCOMPARE(luaGlobalString(hosts.at(0), "headlessSttErrors"), qsl("1"));
        QCOMPARE(luaGlobalString(hosts.at(1), "headlessSttErrors"), qsl("0"));
        QCOMPARE(luaGlobalString(hosts.at(1), "headlessSttNested"), qsl("1"));
    }

    // The same past a nested call, for what the engine raises itself rather than a refusal naming its caller
    void test_anEngineFaultReachesItsCallerPastANestedCallWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        const auto restoreFactory = qScopeGuard([]() {
            SpeechRecognizerFactory::setFactoryOverride(nullptr);
        });
        QVERIFY2(installHeadlessStandInEngine(), "The stand-in engine was not installed.");
        // With only the built-in macOS engine available, B's call asks for that one and would replace the
        // stand-in mid-load, unhooking its fault; a real built-in engine is kept, and so is the stand-in now
        SpeechRecognizerFactory::setFactoryOverride([](QObject*) -> SpeechRecognizer* {
            return nullptr;
        });
        const QStringList hostnames{qsl("Test-Headless-Host-Speech-Fault-A"), qsl("Test-Headless-Host-Speech-Fault-B")};
        const auto closeProfiles = qScopeGuard([&hostnames]() {
            for (const QString& hostname : hostnames) {
                if (HostManager::self()->hostLoaded(hostname)) {
                    HostManager::self()->deleteHost(hostname);
                }
            }
        });
        static QStringList sEvents;
        sEvents.clear();
        QList<Host*> hosts;
        for (const QString& hostname : hostnames) {
            QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
            Host* host = HostManager::self()->getHost(hostname);
            QVERIFY2(host, "The profile is not in the pool.");
            lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "recordSttEvent", [](lua_State* L) -> int {
                sEvents << qsl("%1:%2:%3").arg(getHostFromLua(L).getName(), QString::fromUtf8(lua_tostring(L, 1)), QString::fromUtf8(lua_tostring(L, 2)));
                return 0;
            });
            host->registerAnonymousEventHandler(qsl("sysSTTStateChanged"), qsl("recordSttEvent"));
            host->registerAnonymousEventHandler(qsl("sysSTTError"), qsl("recordSttEvent"));
            hosts << host;
        }
        // B reacts to the capability change A's load announces with a call that raises nothing
        hosts.at(1)->registerAnonymousEventHandler(qsl("sysSTTCapabilitiesChanged"), qsl("onHeadlessSttCapabilities"));
        QVERIFY(hosts.at(1)->getLuaInterpreter()->compileAndExecuteScript(
                qsl("headlessSttNested = 0 function onHeadlessSttCapabilities() headlessSttNested = headlessSttNested + 1 stt.setVocabulary({'north'}) end")));
        standInEngine()->setBiasing(false);
        standInEngine()->failNextLoad();

        QVERIFY(hosts.at(0)->getLuaInterpreter()->compileAndExecuteScript(qsl("local ok, why = stt.init([[%1]]) headlessSttLoad = why or 'loaded'").arg(mConfigDir.path())));
        QCOMPARE(luaGlobalString(hosts.at(0), "headlessSttLoad"), qsl("failed to initialize model from: %1").arg(mConfigDir.path()));
        QCOMPARE(luaGlobalString(hosts.at(1), "headlessSttNested"), qsl("1"));
        QCOMPARE(sEvents, QStringList({qsl("%1:sysSTTStateChanged:error").arg(hostnames.at(0)), qsl("%1:sysSTTError:the stand-in could not load its model").arg(hostnames.at(0))}));
    }

    // As loading a model, then closing a tab and the last profile, do in SpeechAcrossProfilesTest
    void test_speechToTextListensAndClosesWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        const auto restoreFactory = qScopeGuard([]() {
            SpeechRecognizerFactory::setFactoryOverride(nullptr);
        });
        QVERIFY2(installHeadlessStandInEngine(), "The stand-in engine was not installed.");
        TSpeechBridge* bridge = TSpeechBridge::instance();

        const QString closedHostname = qsl("Test-Headless-Host-Speech-Closed");
        const QString lastHostname = qsl("Test-Headless-Host-Speech-Last");
        QVERIFY2(HostManager::self()->addHost(closedHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(HostManager::self()->addHost(lastHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        const auto closeProfiles = qScopeGuard([&closedHostname, &lastHostname]() {
            for (const QString& hostname : {closedHostname, lastHostname}) {
                if (HostManager::self()->hostLoaded(hostname)) {
                    HostManager::self()->deleteHost(hostname);
                }
            }
        });
        Host* closed = HostManager::self()->getHost(closedHostname);
        Host* last = HostManager::self()->getHost(lastHostname);
        QVERIFY2(closed && last, "The profiles are not in the pool.");
        // Kept outside the profiles' Lua, which goes with them
        static QStringList sEvents;
        sEvents.clear();
        for (Host* host : {closed, last}) {
            lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "recordSttEvent", [](lua_State* L) -> int {
                const QString event = QString::fromUtf8(lua_tostring(L, 1));
                sEvents << qsl("%1:%2").arg(getHostFromLua(L).getName(), event == qsl("sysSTTError") ? event : QString::fromUtf8(lua_tostring(L, 2)));
                return 0;
            });
            host->registerAnonymousEventHandler(qsl("sysSTTStateChanged"), qsl("recordSttEvent"));
            host->registerAnonymousEventHandler(qsl("sysSTTError"), qsl("recordSttEvent"));
        }

        // Nobody holds the microphone, so what the engine says during each of these goes only to the profile
        // that made the call, which alternates
        const QList<QPair<Host*, QString>> calls{{closed, qsl("stt.setVocabulary({'north'})")},
                                                 {last, qsl("stt.setSensitivity('short')")},
                                                 {closed, qsl("stt.close()")},
                                                 {last, qsl("stt.init([[%1]])").arg(mConfigDir.path())},
                                                 {closed, qsl("assert(stt.start())")}};
        for (const auto& [host, call] : calls) {
            QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(call), qPrintable(call));
        }
        QCOMPARE(bridge->microphoneOwner(), closed);
        QCOMPARE(sEvents,
                 QStringList({qsl("%1:sysSTTError").arg(closedHostname),
                              qsl("%1:sysSTTError").arg(lastHostname),
                              qsl("%1:uninitialized").arg(closedHostname),
                              qsl("%1:ready").arg(lastHostname),
                              qsl("%1:listening").arg(closedHostname)}));

        // As a package that listens all the time does, which the GUI's closing profile, its scripts stopped, never gets to
        QVERIFY(closed->getLuaInterpreter()->compileAndExecuteScript(qsl("function onSpeechReady(_, state) if state == 'ready' then stt.start() end end")));
        closed->registerAnonymousEventHandler(qsl("sysSTTStateChanged"), qsl("onSpeechReady"));
        sEvents.clear();
        HostManager::self()->deleteHost(closedHostname);
        QVERIFY2(!standInEngine()->listening(), "Closing the profile holding the microphone left its session running.");
        QCOMPARE(bridge->microphoneOwner(), nullptr);
        QVERIFY2(standInEngine()->initialized(), "The engine was released while a profile was still open.");
        QCOMPARE(sEvents, QStringList());

        // Only closing the last profile releases the engine. This one closes itself from its exit handler,
        // as the reopen tests do, so it is still alive when the release comes, and must not be told of it.
        lua_register(last->getLuaInterpreter()->getLuaGlobalState(), "dropProfile", [](lua_State* L) -> int {
            HostManager::self()->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        last->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onSpeechLastExit"));
        QVERIFY(last->getLuaInterpreter()->compileAndExecuteScript(qsl("function onSpeechLastExit() dropProfile(getProfileName()) end headlessSttStarted = tostring(stt.start())")));
        QCOMPARE(luaGlobalString(last, "headlessSttStarted"), qsl("true"));
        // Earlier tests' profiles, a few rounds over in case one's exit handler opens another
        for (int round = 0; round < 3 && HostManager::self()->getHostCount() > 1; ++round) {
            for (const auto& pHost : HostManager::self()->hostList()) {
                if (pHost->getName() != lastHostname) {
                    HostManager::self()->deleteHost(pHost->getName());
                }
            }
        }
        QVERIFY2(HostManager::self()->getHostCount() == 1, "An earlier test left a profile that will not close, so this one cannot be the last.");
        sEvents.clear();
        const QPointer<SpeechRecognizer> engine = standInEngine();
        HostManager::self()->deleteHost(lastHostname);
        QCOMPARE(HostManager::self()->getHostCount(), 0);
        QVERIFY2(engine, "The engine was destroyed rather than released.");
        QVERIFY2(!engine->initialized(), "The engine kept its resources after the last profile closed.");
        QCOMPARE(sEvents, QStringList());
    }

    // Run twice: here, where Mudlet.ini holds none of these keys, and by HeadlessHostSmokeTestWithMudletIni,
    // where initTestCase() wrote each of them before the pool existed
    void test_profileWithNoMainWindowTakesItsSettingsFromMudletIni()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QSettings* settings = MudletApp::getQSettings();
        QVERIFY2(settings, "No settings store exists with no main window.");
        QCOMPARE(settings->fileName(), qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()));
        const bool fromIni = mudletIniWritten();
        const QStringList keys{qsl("autosaveIntervalMinutes"),
                               qsl("mapCenterSmallAreas"),
                               qsl("autoLoginUsernameDelay"),
                               qsl("autoLoginPasswordDelay"),
                               qsl("triggerMatchThreads"),
                               qsl("triggerMatchThreshold"),
                               qsl("enableMuteAPI"),
                               qsl("enableMuteGame"),
                               qsl("reportMapIssuesToConsole"),
                               qsl("storePasswordsSecurely")};
        for (const QString& key : keys) {
            QVERIFY2(settings->contains(key) == fromIni, qPrintable(key));
        }

        const QString hostname = qsl("Test-Headless-Host-Mudlet-Ini");
        // Where a profile keeps its password when Mudlet.ini says not to use secure storage
        QVERIFY(MudletApp::writeProfileData(hostname, qsl("password"), qsl("inistored")).first);
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const auto deleteHost = qScopeGuard([&hostname]() {
            HostManager::self()->deleteHost(hostname);
        });
        QCOMPARE(host->mMapperCenterSmallAreas, fromIni);
        QCOMPARE(MudletApp::storingPasswordsSecurely(), !fromIni);
        QCOMPARE(CredentialManager::profileStoragePreferred(), fromIni);
        QCOMPARE(host->getPass(), fromIni ? qsl("inistored") : QString());

        QCOMPARE(HostManager::self()->media().apiMuted(), fromIni);
        QCOMPARE(HostManager::self()->media().gameMuted(), fromIni);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("iniMutes = tostring(getConfig('muteMediaAPI')) .. ' ' .. tostring(getConfig('muteMediaGame'))")));
        QCOMPARE(luaGlobalString(host, "iniMutes"), fromIni ? qsl("true true") : qsl("false false"));

        // An exit to a room that is not there is reported on the console only when Mudlet.ini asks for it
        constexpr int missingRoomId = 4242;
        TMap* map = host->mpMap.data();
        QVERIFY(map->addRoom(1));
        map->mpRoomDB->getRoom(1)->setUp(missingRoomId);
        map->audit();
        QVERIFY2(mainBufferHolds(host, qsl("Map audit starting")), "The audit never reached the main console model.");
        QCOMPARE(mainBufferHolds(host, qsl("to: %1 but that room does not exist").arg(missingRoomId)), fromIni);
        QVERIFY2(map->mpRoomDB->getRoom(1)->getUp() == -1, "The audit did not find the missing room.");

        // A second pool leaves them alone, as reading them again would undo a change made since
        {
            const auto restore = qScopeGuard([fromIni]() {
                TMap::smShowMapAuditErrors = fromIni;
            });
            TMap::smShowMapAuditErrors = !fromIni;
            const HostManager secondPool;
            QCOMPARE(TMap::smShowMapAuditErrors, !fromIni);
        }

        // The map autosave is a timer the profile starts on itself
        const std::chrono::minutes autosave(fromIni ? scmAutosaveMinutes : 2);
        const auto timers = QAbstractEventDispatcher::instance()->timersForObject(host);
        QVERIFY2(std::any_of(timers.cbegin(),
                             timers.cend(),
                             [&autosave](const auto& timer) {
                                 return timer.interval == autosave;
                             }),
                 "The map autosave did not take autosaveIntervalMinutes.");

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        QByteArray received;
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [&received, client]() {
                received.append(client->readAll());
            });
        });
        host->setLogin(qsl("inihero"));
        host->setPass(qsl("inisecret"));
        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());
        QTRY_VERIFY_WITH_TIMEOUT(received.contains("inihero") && received.contains("inisecret"), 10000);
        QList<int> intervals;
        for (const auto* timer : host->mTelnet.findChildren<QTimer*>()) {
            intervals << timer->interval();
        }
        QVERIFY2(intervals.contains(fromIni ? scmUsernameDelayMs : 2000), "The login did not wait autoLoginUsernameDelay.");
        QVERIFY2(intervals.contains(fromIni ? scmPasswordDelayMs : 1000), "The password did not wait autoLoginPasswordDelay.");
        host->mTelnet.disconnectIt();
        QTRY_COMPARE_WITH_TIMEOUT(host->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState, 10000);

        // The environment outranks Mudlet.ini, so a developer pinning the pool for a run gets what they asked for
        if (qEnvironmentVariableIsSet("MUDLET_MATCH_THREADS") || qEnvironmentVariableIsSet("MUDLET_MATCH_THRESHOLD")) {
            QSKIP("A MUDLET_MATCH_* variable outranks Mudlet.ini for the trigger match pool.");
        }
        const TriggerMatchPool& pool = TriggerMatchPool::instance();
        const int cores = std::max(1, QThread::idealThreadCount());
        const int threads = fromIni ? std::min(scmMatchThreads, cores) : 0;
        QCOMPARE(pool.threshold(), fromIni ? scmMatchThreshold : 128);
        QCOMPARE(pool.workerCount(), threads < 2 ? 0 : threads);
    }

    void test_heldLineCommitsBeforeDisconnectWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        // Prose that runs right up to the wrap column, so undoing server wrap holds it back
        const QByteArray heldLine = "Welcome traveller, the gate stands open and";
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            client->write(heldLine + "\r\n");
            client->disconnectFromHost();
        });

        QVERIFY2(HostManager::self()->addHost(mHeldLineHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mHeldLineHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->mUndoServerWrap = true;
        host->mUndoServerWrapWidth = static_cast<int>(heldLine.size());
        // Long enough that only the disconnect, not either timer, can commit the held line
        host->mTelnet.setPostingTimeout(60000);
        host->mServerWrapFlushTimer.setInterval(60000);
        QSignalSpy held(&host->mainConsoleModel().mNotifier, &TConsoleModelNotifier::serverWrapLineHeld);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessHeldLine = 'none'; tempTrigger("the gate stands open", [[headlessHeldLine = line]]))lua")));
        QString heldLineAtDisconnect;
        // Scoped to this test, as the profile outlives the locals the slot writes to
        QObject receiver;
        connect(&host->mTelnet, &cTelnet::signal_disconnected, &receiver, [&]() {
            heldLineAtDisconnect = luaGlobalString(host, "headlessHeldLine");
        });

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());

        QTRY_VERIFY_WITH_TIMEOUT(!heldLineAtDisconnect.isEmpty(), 10000);
        QCOMPARE(held.count(), 1);
        QCOMPARE(heldLineAtDisconnect, QString::fromUtf8(heldLine));
    }

#if !defined(QT_NO_SSL)
    void test_untrustedCertificateWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        if (QSslSocket::activeBackend() != QLatin1String("openssl")) {
            QSKIP("Serving a certificate from an in-memory PEM key is only relied on with the OpenSSL backend.");
        }

        // Self-signed, so the profile refuses it; valid until 2126 so it never fails as expired instead
        static constexpr char certificatePem[] = R"(-----BEGIN CERTIFICATE-----
MIIBfzCCASWgAwIBAgIUNVnXBt8EptvmkFkhxe/ibqLOLRIwCgYIKoZIzj0EAwIw
FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTAwOTIxNTA1OFoYDzIxMjYwOTE1
MjE1MDU4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO
PQMBBwNCAASh4Kz7zVzveu+VpaQSoceVFsH6I6qOfbYT0tapBHTFGBkf6NgxBGen
wL5TDeL9g3w57+FWiHtIKUylQhCoNb20o1MwUTAdBgNVHQ4EFgQUdeivXGb0CJyG
TeJIhMFeiKCyPV0wHwYDVR0jBBgwFoAUdeivXGb0CJyGTeJIhMFeiKCyPV0wDwYD
VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEA4ktl+ztKx3yhNaOQRVvo
t7BeROX3QMOcjmtFD1z7gMMCIBCdl4RZ9RpZqyngwieUaEVXNou189dJOrb5/4Iu
62HC
-----END CERTIFICATE-----)";
        static constexpr char keyPem[] = R"(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgrC6UFloRMFbttYkA
mx8Y3UWOzQ/JtyVfFEEK23/u4iGhRANCAASh4Kz7zVzveu+VpaQSoceVFsH6I6qO
fbYT0tapBHTFGBkf6NgxBGenwL5TDeL9g3w57+FWiHtIKUylQhCoNb20
-----END PRIVATE KEY-----)";
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        configuration.setLocalCertificate(QSslCertificate(QByteArray(certificatePem)));
        configuration.setPrivateKey(QSslKey(QByteArray(keyPem), QSsl::Ec));
        QSslServer server;
        server.setSslConfiguration(configuration);
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");

        QVERIFY2(HostManager::self()->addHost(mSecureHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mSecureHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->mSslTsl = true;
        QSignalSpy disconnected(&host->mTelnet, &cTelnet::signal_disconnected);

        // Not 127.0.0.1: a secure connect dials the name the lookup returns, and Windows
        // reverse-resolves 127.0.0.1 to the machine's own name, whose addresses this server is not on
        host->mTelnet.connectIt(qsl("localhost"), server.serverPort());

        // The app opens the profile's connection preferences here, which needs a main window
        QTRY_VERIFY_WITH_TIMEOUT(!disconnected.isEmpty(), 10000);
        QVERIFY2(!host->mTelnet.getSslErrors().isEmpty(), "The connection did not fail on the certificate.");
        QVERIFY2(mainBufferHolds(host, qsl("self-signed")), "Why the connection was refused never reached the main console model.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The refused connection created a widget.");
    }
#endif

    // A deleted window's model outlives the call that deleted it, as a real view's widget does, and goes
    // when deferred deletes run; twice, as each release has to queue the next.
    void test_deletedWindowModelGoesWithDeferredDeletes()
    {
        const QString hostname = qsl("Test-Headless-Host-Release");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        for (int round = 0; round < 2; ++round) {
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(createMiniConsole('releasedMini', 0, 0, 10, 10))")));
            TConsoleModel* model = host->windowRegistry().subConsoleModel(qsl("releasedMini"));
            QVERIFY2(model, "The mini console has no model.");
            const QPointer<TConsoleModelNotifier> notifier = &model->mNotifier;
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(deleteMiniConsole('releasedMini'))")));
            QVERIFY2(notifier, "The model went before deferred deletes ran.");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QVERIFY2(!notifier, "The model outlived deferred deletes.");
        }
    }

    void test_profileGetsNullAppViewWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        TAppFrontend* app = TAppFrontend::instance();
        QVERIFY2(app, "TAppFrontend::instance() must never be null.");
        QVERIFY(!app->getActiveHost());
        QCOMPARE(app->profileTabIndex(mHostname), -1);
        QVERIFY(app->getOpenFileName(qsl("title"), QDir::tempPath()).isEmpty());
        QVERIFY(!app->quitting());

        const QString hostname = qsl("Test-Headless-Host-App-View");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // A profile closing earlier in this run saved its layout, which loadWindowLayout() would answer true for
        QFile::remove(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat")));
        // Each of these calls the app view without asking whether there is one
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessAppResult = 'not run'
local ok, err = pcall(function()
  local tab, tabMsg = getProfileTabNumber()
  assert(tab == nil and type(tabMsg) == "string", "getProfileTabNumber did not answer nil and a message")
  assert(openWebPage("about:blank") == false, "openWebPage did not answer false")
  assert(loadWindowLayout() == false, "loadWindowLayout did not answer false")
  assert(showNotification("headless title", "headless text") == true, "showNotification did not answer true")
end)
headlessAppResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessAppResult"), qsl("ok"));
    }

    void test_scriptsSeeNoMainWindowAsTheyDidBeforeTheNullAppView()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Answers");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessAnswersResult = 'not run'
local ok, err = pcall(function()
  assert(invokeFileDialog(true, "headless") == "", "invokeFileDialog did not answer an empty string")
  alert(0)
  local shown, shownMsg = showToolBar("headless missing toolbar")
  assert(shown == nil and type(shownMsg) == "string", "showToolBar did not answer nil and a message")
end)
headlessAnswersResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessAnswersResult"), qsl("ok"));
    }

    // The answers are AddonCommand_spec.lua's for the same calls, but for what only the main window's bars, its own
    // shortcuts and its widgets' can refuse
    void test_addonCommandsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Addon-Commands");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommands = 'not run'
local ok, err = pcall(function()
  local id = addCommand({name = "Speech", menuPath = "Speech/Voices", shortcut = "Ctrl+Alt+F5, Ctrl+Alt+F6, Ctrl+Alt+F7, Ctrl+Alt+F8", tooltip = "<b>talk</b>"})
  assert(type(id) == "number", "addCommand did not hand back a number: " .. tostring(id))
  local second = addCommand({name = "Speech", menuPath = "Speech/Voices"})
  assert(type(second) == "number" and second ~= id, "a second command did not get an id of its own")
  assert(type(addCommand({name = "Comma", shortcut = "Ctrl+,, Ctrl+Alt+F1, Ctrl+Alt+F2, Ctrl+Alt+F3"})) == "number", "a sequence of four steps starting with the comma key was refused")
  headlessCommandId = id

  assert(disableCommand(id) == true, "disableCommand did not answer true")
  assert(enableCommand(id) == true, "enableCommand did not answer true")
  assert(setCommandChecked(id, true) == true, "setCommandChecked did not answer true")
  assert(setCommandPinned(id, true) == true, "setCommandPinned did not answer true")
  assert(setCommandIcon(id, "/no/such/icon.png") == true, "setCommandIcon did not answer true")
  assert(setCommandTooltip(id, [[Damage 5 <10 & rising]]) == true, "setCommandTooltip did not answer true")
  assert(setCommandPulse(id, true, "#22aa44", "#116622", 700) == true, "setCommandPulse did not answer true")
  assert(setCommandPulse(id, false) == true, "stopping the pulse did not answer true")
  local badColour, badColourMsg = setCommandPulse(id, true, "reed", "#116622")
  assert(badColour == nil and badColourMsg == '"reed" is not a colour Qt recognises', "an unknown colour was not refused: " .. tostring(badColourMsg))
  local menuOnly = addCommand({name = "Menu only", surfaces = "menu"})
  local noButton, noButtonMsg = setCommandPulse(menuOnly, true)
  assert(noButton == nil and noButtonMsg == "that command is not on the toolbar, and a pulse has nothing to colour without a button", "a menu-only command took a pulse: " .. tostring(noButtonMsg))
  assert(type(addCommand({name = "Toolbar only", surfaces = "toolbar"})) == "number", "a toolbar-only command was refused")

  local function refused(request, expected)
    local refusedId, why = addCommand(request)
    assert(refusedId == nil and why == expected, request.name .. " answered " .. tostring(refusedId) .. ", " .. tostring(why))
  end
  refused({name = "ToolbarPath", surfaces = "toolbar", menuPath = "Spec"}, 'a menu path needs a menu item to go in, so it cannot be used with surfaces = "toolbar"')
  refused({name = "ToolbarKey", surfaces = "toolbar", shortcut = "Ctrl+Alt+F11"}, 'a shortcut needs a menu item to hang on, so it cannot be used with surfaces = "toolbar"')
  refused({name = "Typo", shortcut = "Ctrl+Alt+F10, Ctrl+Shft+B"}, "that is not a key sequence Qt understands")
  refused({name = "FiveSteps", shortcut = "Ctrl++, Ctrl+Alt+F1, Ctrl+Alt+F2, Ctrl+Alt+F3, Ctrl+Alt+F4"}, "a key sequence can be 4 step(s) long at most")
  refused({name = "Speech"}, '"Speech" is already a submenu here, so a command cannot take that label too')
  refused({name = "Deeper", menuPath = "Speech/Voices/Speech/More"}, '"Speech" is already a command in this menu, so it cannot also be a submenu')
  assert(type(addCommand({name = "Speech", surfaces = "toolbar"})) == "number", "a toolbar-only command was refused the label of a submenu")

  local function refusedFor(request, holder)
    local refusedId, why = addCommand(request)
    assert(refusedId == nil and tostring(why):find(holder, 1, true), request.name .. " answered " .. tostring(refusedId) .. ", " .. tostring(why))
  end
  assert(type(addCommand({name = "KeyHolder", shortcut = "Ctrl+Alt+J"})) == "number", "a free key was refused")
  refusedFor({name = "WantsKey", shortcut = "Ctrl+Alt+J"}, 'is already taken by "KeyHolder"')
  -- Alt and F9: mudlet.keymodifier and mudlet.key come with mudlet-lua, which this run has not loaded
  permKey("HeadlessKeyClash", "", 0x08000000, 0x01000038, [[echo("bound")]])
  refusedFor({name = "WantsBoundKey", shortcut = "Alt+F9"}, 'is already taken by the "HeadlessKeyClash" key binding')

  assert(removeCommand(second) == true, "removeCommand did not answer true")
  for name, call in pairs({
    removeCommand = function() return removeCommand(second) end,
    enableCommand = function() return enableCommand(second) end,
    disableCommand = function() return disableCommand(second) end,
    setCommandChecked = function() return setCommandChecked(second, true) end,
    setCommandIcon = function() return setCommandIcon(second, "") end,
    setCommandTooltip = function() return setCommandTooltip(second, "nothing") end,
    setCommandPinned = function() return setCommandPinned(second, true) end,
    setCommandPulse = function() return setCommandPulse(second, true) end,
  }) do
    assert(call() == false, name .. " did not answer false for a removed command")
  end
end)
headlessCommands = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommands"), qsl("ok"));
        const QString idText = luaGlobalString(host, "headlessCommandId");
        QVERIFY2(!idText.isEmpty(), "The command's id was not kept.");

        // Only its own profile addresses a command
        const QString otherHostname = qsl("Test-Headless-Host-Addon-Commands-Other");
        QVERIFY2(HostManager::self()->addHost(otherHostname, QString(), QString(), QString()), "Could not create a second profile with no main window.");
        Host* other = HostManager::self()->getHost(otherHostname);
        QVERIFY2(other, "The second profile is not in the pool.");
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl("otherAnswer = tostring(enableCommand(%1)) .. ' ' .. tostring(removeCommand(%1))").arg(idText)));
        QCOMPARE(luaGlobalString(other, "otherAnswer"), qsl("false false"));
        QVERIFY(other->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local otherId, otherWhy = addCommand({name = "OtherKey", shortcut = "Ctrl+Alt+J"})
otherKeyAnswer = tostring(otherId) .. " " .. tostring(otherWhy)
)lua")));
        QVERIFY2(luaGlobalString(other, "otherKeyAnswer").endsWith(qsl("is already taken by a command from another profile")), qPrintable(luaGlobalString(other, "otherKeyAnswer")));
        HostManager::self()->deleteHost(otherHostname);

        // Kept for a frontend to show, and activated as a click on the main window's would
        auto* app = static_cast<TNullAppFrontend*>(TAppFrontend::instance());
        const int commandId = idText.toInt();
        const TNullAppFrontend::AddonCommand command = app->addonCommands().value(commandId);
        QCOMPARE(command.request.name, qsl("Speech"));
        QCOMPARE(command.request.menuPath, qsl("Speech/Voices"));
        QCOMPARE(command.icon, qsl("/no/such/icon.png"));
        QCOMPARE(command.tooltip, qsl("Damage 5 <10 & rising"));
        QVERIFY(command.enabled && command.checkable && command.checked && command.pinned);
        QVERIFY(!command.pulseEnabled);
        QCOMPARE(command.pulseColor1, qsl("#22aa44"));
        QCOMPARE(command.pulseInterval, 700);
        host->registerAnonymousEventHandler(qsl("sysCommandClicked"), qsl("onHeadlessCommandClicked"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
clickedCommands = ""
function onHeadlessCommandClicked(_, clicked) clickedCommands = clickedCommands .. type(clicked) .. clicked .. " " end
)lua")));
        QVERIFY(app->activateAddonCommand(commandId));
        QCOMPARE(luaGlobalString(host, "clickedCommands"), qsl("number%1 ").arg(commandId));
        QVERIFY2(!app->addonCommands().value(commandId).checked, "Activating a ticked command did not untick it.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("disableCommand(%1)").arg(commandId)));
        QVERIFY2(!app->activateAddonCommand(commandId), "A disabled command was activated.");
        QVERIFY2(!app->activateAddonCommand(-1), "An unknown command was activated.");
        QCOMPARE(luaGlobalString(host, "clickedCommands"), qsl("number%1 ").arg(commandId));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("enableCommand(%1)").arg(commandId)));

        // A command goes with its package, and every one of a profile's with a reset
        const TAppFrontend::CommandRequest request{.name = qsl("packaged")};
        QString error;
        const int kept = app->addAddonCommand(request, host, qsl("keptPackage"), error);
        const int packaged = app->addAddonCommand(request, host, qsl("removedPackage"), error);
        QVERIFY(kept > 0 && packaged > 0);
        emit host->signal_packageRemoved(qsl("removedPackage"));
        QVERIFY2(!app->setAddonCommandEnabled(packaged, true, host), "A command outlived its package.");
        QVERIFY2(app->setAddonCommandEnabled(kept, true, host), "Removing one package took another's command.");
        QVERIFY2(app->setAddonCommandEnabled(idText.toInt(), true, host), "Removing a package took the profile's own command.");
        emit host->signal_profileResetting();
        QVERIFY2(!app->setAddonCommandEnabled(kept, true, host), "A command outlived its profile's reset.");
        QVERIFY2(!app->setAddonCommandEnabled(idText.toInt(), true, host), "The profile's own command outlived its reset.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The commands created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // As the GUI answers with no saved settings
    void test_mainWindowSettingsRoundTripWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Settings");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSettingsResult = 'not run'
local ok, err = pcall(function()
  local function check(label, want, ...)
    local n = select("#", ...)
    local got = ...
    assert(n == 1 and got == want, label .. " answered " .. n .. " values: " .. tostring(got))
  end
  local key = "showTabConnectionIndicators"
  check("getConfig default", false, getConfig(key))
  check("setConfig true", true, setConfig(key, true))
  check("getConfig after true", true, getConfig(key))
  check("setConfig false", true, setConfig(key, false))
  check("getConfig after false", false, getConfig(key))
end)
headlessSettingsResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSettingsResult"), qsl("ok"));
    }

    // As Other_spec.lua pins for the GUI: one event per real change, none for a write of the value held
    void test_settingChangesRaiseEventsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Setting-Events");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
        host->registerAnonymousEventHandler(qsl("sysSettingChanged"), qsl("onHeadlessSettingChanged"));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSettingEvents = 'not run'
local events = {}
function onHeadlessSettingChanged(_, key, value)
  events[#events + 1] = {key = key, value = value, readBack = getConfig(key)}
end
local ok, err = pcall(function()
  assert(getConfig("muteMediaAPI") == false and getConfig("muteMediaGame") == false, "media starts muted")
  local keys = {"muteMediaAPI", "muteMediaGame", "compactInputLine", "mapperPanelVisible", "enableClosedCaption", "advertiseScreenReader", "announceIncomingText"}
  for _, key in ipairs(keys) do
    local start = getConfig(key)
    for _, step in ipairs({{not start, 1}, {not start, 0}, {start, 1}}) do
      local value, want = step[1], step[2]
      events = {}
      assert(setConfig(key, value) == true, "setConfig " .. key .. " refused")
      assert(#events == want, key .. " set to " .. tostring(value) .. " raised " .. #events .. " events, not " .. want)
      if want == 1 then
        local event = events[1]
        assert(event.key == key and event.value == value and event.readBack == value,
          key .. " raised " .. tostring(event.key) .. " = " .. tostring(event.value) .. ", read back " .. tostring(event.readBack))
      end
    end
  end
end)
headlessSettingEvents = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSettingEvents"), qsl("ok"));
    }

    // The answers and events are those a GUI probe of the same calls gave, with the profile names changed
    void test_appStyleSheetWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Style");
        const QString otherHostname = qsl("Test-Headless-Host-App-Style-Other");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(HostManager::self()->addHost(otherHostname, QString(), QString(), QString()), "Could not create a second profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        Host* otherHost = HostManager::self()->getHost(otherHostname);
        QVERIFY2(host && otherHost, "The profiles are not in the pool.");

        const QString recorder = qsl(R"lua(
headlessStyleLog = {}
function onHeadlessAppStyleSheet(...)
  local parts = {}
  for i = 1, select("#", ...) do parts[#parts + 1] = tostring((select(i, ...))) end
  headlessStyleLog[#headlessStyleLog + 1] = "  event: " .. table.concat(parts, ", ") .. " (" .. select("#", ...) .. " args)"
end
)lua");
        for (Host* profile : {host, otherHost}) {
            QVERIFY2(profile->getLuaInterpreter()->compileAndExecuteScript(recorder), "The Lua chunk did not run.");
            // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
            profile->registerAnonymousEventHandler(qsl("sysAppStyleSheetChange"), qsl("onHeadlessAppStyleSheet"));
        }

        const QString calls = qsl(R"lua(
local function call(label, f, ...)
  local res = {pcall(f, ...)}
  local parts = {}
  for i = 1, table.maxn(res) do parts[#parts + 1] = tostring(res[i]) end
  headlessStyleLog[#headlessStyleLog + 1] = label .. " -> " .. table.concat(parts, ", ")
end
local sheet = "QToolTip { color: #102030; }"
call("setAppStyleSheet(sheet)", setAppStyleSheet, sheet)
call("setAppStyleSheet(sheet, tag)", setAppStyleSheet, sheet, "probeTag")
call("setAppStyleSheet(empty)", setAppStyleSheet, "")
call("setAppStyleSheet()", setAppStyleSheet)
call("setAppStyleSheet(number)", setAppStyleSheet, 5)
call("setAppStyleSheet(table)", setAppStyleSheet, {})
call("setAppStyleSheet(sheet, number)", setAppStyleSheet, sheet, 7)
call("setAppStyleSheet(sheet, nil)", setAppStyleSheet, sheet, nil)
call("setAppStyleSheet(bool)", setAppStyleSheet, true)
)lua");
        const QString styleSheetBefore = qApp->styleSheet();
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(calls), "The Lua chunk did not run.");

        const QString expected = qsl(R"(  event: sysAppStyleSheetChange, , %1 (3 args)
setAppStyleSheet(sheet) -> true, true
  event: sysAppStyleSheetChange, probeTag, %1 (3 args)
setAppStyleSheet(sheet, tag) -> true, true
  event: sysAppStyleSheetChange, , %1 (3 args)
setAppStyleSheet(empty) -> true, true
setAppStyleSheet() -> false, setAppStyleSheet: bad argument #1 type (style sheet as string expected, got no value!)
  event: sysAppStyleSheetChange, , %1 (3 args)
setAppStyleSheet(number) -> true, true
setAppStyleSheet(table) -> false, setAppStyleSheet: bad argument #1 type (style sheet as string expected, got table!)
  event: sysAppStyleSheetChange, 7, %1 (3 args)
setAppStyleSheet(sheet, number) -> true, true
setAppStyleSheet(sheet, nil) -> false, setAppStyleSheet: bad argument #2 type (tag as string expected, got nil!)
setAppStyleSheet(bool) -> false, setAppStyleSheet: bad argument #1 type (style sheet as string expected, got boolean!))")
                                         .arg(hostname);
        for (Host* profile : {host, otherHost}) {
            QVERIFY2(profile->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessStyleLogText = table.concat(headlessStyleLog, "\n"))lua")), "The Lua chunk did not run.");
        }
        const QString answered = luaGlobalString(host, "headlessStyleLogText");
        QCOMPARE(answered, expected);

        // Every profile hears of it, not only the one that set it, and each is told which one that was
        QStringList otherExpected;
        for (const QString& line : expected.split(QChar::LineFeed)) {
            if (line.startsWith(qsl("  event: "))) {
                otherExpected << line;
            }
        }
        const QString otherHeard = luaGlobalString(otherHost, "headlessStyleLogText");
        QCOMPARE(otherHeard, otherExpected.join(QChar::LineFeed));

        QVERIFY2(qApp->styleSheet() == styleSheetBefore, "The style sheet was applied with no main window to style.");
    }

    // The lines and trigger match are those a GUI probe of the same feeds gave
    void test_stalledMxpTagIsWrittenOutWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Mxp-Stall");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessStallLog = {}
local function record(text) headlessStallLog[#headlessStallLog + 1] = text end
local mark = getLastLineNumber("main")
function headlessStallTail(label)
  record(label)
  for _, l in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do record("  line: [" .. l .. "]") end
end
record("force -> " .. tostring(setConfig("specialForceMXPProcessorOn", true)))
headlessStallTrigger = tempRegexTrigger("stall", function()
  record("  trigger: [" .. line .. "]")
  headlessStallFired = "yes"
end)
feedTriggers("plain line\n")
feedTriggers("stall <send")
headlessStallTail("fed")
)lua")),
                 "The Lua chunk did not run.");

        // The watchdog gives a stalled tag two of its 1300ms timeouts before it writes it out
        QTest::qWaitFor(
                [host]() {
                    return luaGlobalString(host, "headlessStallFired") == qsl("yes");
                },
                8000);

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessStallTail("after the watchdog")
killTrigger(headlessStallTrigger)
feedTriggers(">done\n")
headlessStallTail("after >done")
setConfig("specialForceMXPProcessorOn", false)
headlessStallLogText = table.concat(headlessStallLog, "\n")
)lua")),
                 "The Lua chunk did not run.");

        QCOMPARE(luaGlobalString(host, "headlessStallLogText"), qsl(R"(force -> true
fed
  line: [plain line]
  line: []
  trigger: [stall <send]
after the watchdog
  line: [plain line]
  line: [stall <send]
  line: []
after >done
  line: [plain line]
  line: [stall <send]
  line: [>done]
  line: [])"));
    }

    // As Media_spec.lua pins for the GUI: with closed captions on, an ended playback is captioned in the main console
    void test_closedCaptionsReachTheMainConsoleWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Captions");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const QString soundFile = qsl("%1/headless-caption.wav").arg(mConfigDir.path());
        QFile file(soundFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(silentWav());
        file.close();

        // Stopped at once, so the backend need not be able to play it: a stop is captioned whether or not the sound had started
        const QString script = qsl(R"lua(
local ok, err = pcall(function()
  assert(setConfig("enableClosedCaption", true), "captions refused")
  assert(playSoundFile({name = [[%1]], key = "headlessCaption", caption = "headless caption"}), "playSoundFile refused")
  assert(stopSounds(), "stopSounds refused")
end)
headlessCaptions = ok and 'ok' or tostring(err)
)lua")
                                       .arg(soundFile);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCaptions"), qsl("ok"));
        QTRY_VERIFY2(mainBufferHolds(host, qsl("[headless caption stops]")), "The stopped sound was not captioned in the main console.");
    }

    // Nothing copies the bundled fonts out to the config directory here, as the executable does for the GUI
    void test_bundledFontsAreAvailableWithNoMainWindow()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there are no bundled fonts to list");
#else
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Fonts");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local ok, err = pcall(function()
  local fonts = getAvailableFonts()
  assert(fonts["Bitstream Vera Sans Mono"], "the bundled default font is not listed")
  assert(fonts["Ubuntu Mono"], "the bundled Ubuntu Mono is not listed")
  local set, why = setConfig("mapSymbolFont", "Bitstream Vera Sans Mono")
  assert(set, "the bundled default was refused as the map symbol font: " .. tostring(why))
  assert(getConfig("mapSymbolFont") == "Bitstream Vera Sans Mono", "the map symbol font reads back as " .. tostring(getConfig("mapSymbolFont")))
end)
headlessFonts = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFonts"), qsl("ok"));

        // The families alone could be fonts this machine has installed itself
        FontManager* fonts = FontManager::self();
        QVERIFY2(fonts, "The profile pool has no font manager.");
        QVERIFY2(fonts->fontAlreadyLoaded(qsl("%1/Vera.ttf").arg(veraFontFolder()), QString()), "The font copied out to the fonts folder was not loaded from there.");
        QVERIFY2(!fonts->fontAlreadyLoaded(qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf"), QString()), "The font copied out to the fonts folder was loaded from the resources too.");
        QVERIFY2(fonts->fontAlreadyLoaded(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), QString()), "A font missing from a copied-out bundle was not loaded from the resources.");
        QVERIFY2(fonts->fontAlreadyLoaded(qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf"), QString()), "A bundle never copied out was not loaded from the resources.");
#endif
    }

    // As the GUI answers the same files
    void test_imageSizeWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Image-Size");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        QTemporaryDir images;
        QVERIFY(images.isValid());
        const auto writeFile = [&images](const QString& name, const QByteArray& content) {
            QFile file(images.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
        };
        QVERIFY(QImage(37, 21, QImage::Format_ARGB32).save(images.filePath(qsl("raster.png"))));
        QVERIFY(writeFile(qsl("document.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"48\"/>")));
        // QImage reads this by its content, which the SVG renderer cannot
        QVERIFY(QImage(5, 9, QImage::Format_ARGB32).save(images.filePath(qsl("raster.svg")), "PNG"));
        // Not named .svgz, so only its content says it is gzipped
        QVERIFY(writeFile(
                qsl("gzipped.svg"),
                QByteArrayLiteral(
                        "\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff\xb3\x29\x2e\x4b\x57\xa8\xc8\xcd\xc9\x2b\xb6\x55\xca\x28\x29\x29\xb0\xd2\xd7\x2f\x2f\x2f\xd7\x2b\x37\xd6\xcb\x2f\x4a\xd7\x37\x32\x30"
                        "\x30\xd0\x07\xaa\x50\x52\x28\xcf\x4c\x29\xc9\xb0\x55\x32\x31\x50\x52\xc8\x48\xcd\x4c\xcf\x28\xb1\x55\x32\x36\x50\xd2\xb7\x03\x00\x54\x4e\x04\xa0\x40\x00\x00\x00")));

        const QString missing = images.filePath(qsl("missing.png"));
        // The refusal is outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
        const QString script = qsl("imageDir = \"%1/\"\nmissingRefusal = \"couldn't retrieve image size, is the location '%2' correct?\"\n").arg(images.path(), missing) + qsl(R"lua(
headlessImageSize = 'not run'
local ok, err = pcall(function()
  local function check(label, wantA, wantB, ...)
    local n = select("#", ...)
    local a, b = ...
    assert(n == 2 and a == wantA and b == wantB, label .. " answered " .. n .. " values: " .. tostring(a) .. ", " .. tostring(b))
  end
  check("PNG", 37, 21, getImageSize(imageDir .. "raster.png"))
  check("SVG", 64, 48, getImageSize(imageDir .. "document.svg"))
  check("raster named .svg", 5, 9, getImageSize(imageDir .. "raster.svg"))
  check("gzipped SVG", 40, 30, getImageSize(imageDir .. "gzipped.svg"))
  check("missing file", nil, missingRefusal, getImageSize(imageDir .. "missing.png"))
  check("empty path", nil, "image location cannot be an empty string", getImageSize(""))
end)
headlessImageSize = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessImageSize"), qsl("ok"));

        // QImage reads SVG too where the qsvg plugin is deployed, which would hide a broken helper from getImageSize
        QVERIFY(TLabelModel::svgCandidate(images.filePath(qsl("document.svg"))));
        QVERIFY(TLabelModel::svgCandidate(images.filePath(qsl("gzipped.svg"))));
        QVERIFY(!TLabelModel::svgCandidate(images.filePath(qsl("raster.svg"))));
        QSvgRenderer renderer;
        QVERIFY(TLabelModel::loadSvg(renderer, images.filePath(qsl("gzipped.svg"))));
        QCOMPARE(renderer.defaultSize(), QSize(40, 30));
    }

    // Text echoed to main takes the profile's colours from birth, as the view applies them when it is
    // built, and then the colours a profile save brings.
    void test_mainConsoleTextTakesTheProfileColours()
    {
        const QString hostname = qsl("Test-Headless-Host-Colours");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString echoAndRead = qsl(R"lua(
echo("main", "\n%1")
moveCursorEnd("main")
selectString("main", "%1", 1)
local fr, fg, fb = getFgColor("main")
local br, bg, bb = getBgColor("main")
deselect("main")
headlessColours = table.concat({fr, fg, fb, br, bg, bb}, ",")
)lua");
        const auto echoedColours = [host, &echoAndRead](const QString& text) {
            host->getLuaInterpreter()->compileAndExecuteScript(echoAndRead.arg(text));
            return luaGlobalString(host, "headlessColours");
        };
        const auto coloursOf = [](const QColor& fg, const QColor& bg) {
            return qsl("%1,%2,%3,%4,%5,%6").arg(fg.red()).arg(fg.green()).arg(fg.blue()).arg(bg.red()).arg(bg.green()).arg(bg.blue());
        };
        QCOMPARE(echoedColours(qsl("fresh text")), coloursOf(QColorConstants::LightGray, QColorConstants::Black));

        const QByteArray save = "<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host>"
                                "<mFgColor>#102030</mFgColor><mBgColor>#405060</mBgColor>"
                                "</Host></HostPackage></MudletPackage>";
        QTemporaryDir saveDir;
        QVERIFY(saveDir.isValid());
        QFile file(saveDir.filePath(qsl("save.xml")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(save), save.size());
        file.close();
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        XMLimport importer(host);
        const auto [imported, importError] = importer.importPackage(&file);
        QVERIFY2(imported, qPrintable(importError));
        QCOMPARE(echoedColours(qsl("loaded text")), coloursOf(QColor(0x10, 0x20, 0x30), QColor(0x40, 0x50, 0x60)));
    }

    // The GUI resolves "main" to the main console before it looks for a label of that name
    void test_setFontPrefersMainConsoleOverALabelNamedMain()
    {
        const QString hostname = qsl("Test-Headless-Host-Font-Label-Main");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        const QFont startFont = host->getDisplayFont();
        const QString startFamily = QFontInfo(startFont).family();

        // setFont() refuses a family this machine does not have, and macOS CI has none of the Linux ones
        const QStringList installed = FontManager::availableFonts();
        QString family;
        for (const QString& candidate : QStringList{qsl("DejaVu Sans"), qsl("Ubuntu Mono"), qsl("Menlo"), qsl("Courier New")} + installed) {
            if (installed.contains(candidate) && !candidate.startsWith(QLatin1Char('.')) && !candidate.contains(QLatin1Char('"')) && !candidate.contains(QLatin1Char('\\'))
                && QFontInfo(host->createFontWithSettings(candidate, startFont.pointSize())).family() != startFamily) {
                family = candidate;
                break;
            }
        }
        QVERIFY2(!family.isEmpty(), "No font family other than the main console one is installed.");

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("labelMainFamily = \"%1\"").arg(family)));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelMain = "not run"
local ok, err = pcall(function()
  assert(createLabel("main", 0, 0, 50, 20, 1) == true, "createLabel did not answer true")
end)
headlessLabelMain = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelMain"), qsl("ok"));
        const auto labelBefore = host->labelFont(qsl("main"));
        QVERIFY2(labelBefore, "The label named main was not made.");

        const bool set = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelMain = "not run"
local ok, err = pcall(function()
  assert(setFont("main", labelMainFamily) == true, "setFont did not answer true for main")
  labelMainSetFont = getFont("main")
end)
headlessLabelMain = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(set, "The Lua chunk setting the font did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelMain"), qsl("ok"));
        const auto labelAfter = host->labelFont(qsl("main"));
        QVERIFY(labelAfter);
        QCOMPARE(labelAfter->family(), labelBefore->family());

        const bool deleted = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelMain = "not run"
local ok, err = pcall(function()
  assert(deleteLabel("main") == true, "deleteLabel did not answer true")
  labelMainAfterDelete = getFont("main")
end)
headlessLabelMain = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(deleted, "The Lua chunk deleting the label did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelMain"), qsl("ok"));
        const QString expected = QFontInfo(host->createFontWithSettings(family, startFont.pointSize())).family();
        QCOMPARE(luaGlobalString(host, "labelMainSetFont"), expected);
        QCOMPARE(luaGlobalString(host, "labelMainAfterDelete"), expected);
        QCOMPARE(QFontInfo(host->getDisplayFont()).family(), expected);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "A profile with no main window made a widget.");
    }

    // As MapDownloadTest's checks of the same downloads in the GUI: the map is stored and loaded,
    // and scripts get sysMapDownloadEvent, whichever kind of map the game serves
    void test_mapDownloadLoadsTheMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        // Nothing renders map progress with no main window, so a warning about it would be a false alarm
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Download");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        TMap* map = host->mpMap.data();
        const QByteArray binaryMap = binaryMapOf(host, 7);
        QVERIFY(!binaryMap.isEmpty());

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            // The request line names the map, and it comes first
            connect(
                    client,
                    &QTcpSocket::readyRead,
                    client,
                    [client, &binaryMap]() {
                        const QByteArray body = client->readAll().startsWith("GET /map.xml ") ? xmlMap() : binaryMap;
                        client->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                        client->disconnectFromHost();
                    },
                    Qt::SingleShotConnection);
        });
        host->registerAnonymousEventHandler(qsl("sysMapDownloadEvent"), qsl("onHeadlessMapDownload"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessMapDownloads = 0; function onHeadlessMapDownload() headlessMapDownloads = headlessMapDownloads + 1 end")));

        map->downloadMap(qsl("http://127.0.0.1:%1/map.xml").arg(server.serverPort()));
        QVERIFY2(map->hasActiveTransferProgress(), "The download did not start.");
        QTRY_VERIFY_WITH_TIMEOUT(!map->hasActiveTransferProgress(), 10000);
        QVERIFY2(map->mpRoomDB->getRoom(3), "The downloaded XML map was not loaded.");
        QCOMPARE(luaGlobalString(host, "headlessMapDownloads"), qsl("1"));

        map->downloadMap(qsl("http://127.0.0.1:%1/map.dat").arg(server.serverPort()));
        QVERIFY2(map->hasActiveTransferProgress(), "The second download did not start, so the first one never finished.");
        QTRY_VERIFY_WITH_TIMEOUT(!map->hasActiveTransferProgress(), 10000);
        QVERIFY2(!mainBufferHolds(host, qsl("failure in parsing")), "The downloaded binary map was reported as unreadable.");
        QVERIFY2(map->mpRoomDB->getRoom(7), "The downloaded binary map was not loaded.");
        QVERIFY2(!map->mpRoomDB->getRoom(3), "The binary map was loaded over the XML one rather than in its place.");
        QCOMPARE(luaGlobalString(host, "headlessMapDownloads"), qsl("2"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Downloading a map created a widget.");
    }

    // As Mapper_spec.lua's checks of saveMap and loadMap in the GUI
    void test_loadMapReadsTheMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Load");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QTemporaryDir mapDir;
        QVERIFY(mapDir.isValid());
        QFile xmlFile(mapDir.filePath(qsl("headless.xml")));
        QVERIFY(xmlFile.open(QIODevice::WriteOnly));
        xmlFile.write(xmlMap());
        xmlFile.close();
        QFile binaryFile(mapDir.filePath(qsl("headless.dat")));
        QVERIFY(binaryFile.open(QIODevice::WriteOnly));
        binaryFile.write(binaryMapOf(host, 7));
        binaryFile.close();
        host->registerAnonymousEventHandler(qsl("mapOpenEvent"), qsl("onHeadlessMapLoadOpened"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessMapLoadOpens = 0; function onHeadlessMapLoadOpened() headlessMapLoadOpens = headlessMapLoadOpens + 1 end")));

        const QString script = qsl("headlessMapDir = [[%1]]\n").arg(mapDir.path()) + qsl(R"lua(
headlessMapLoad = 'not run'
local ok, err = pcall(function()
  local loaded, loadedMsg = loadMap(headlessMapDir .. "/headless.dat")
  assert(loaded == true, "loadMap of a binary map answered " .. tostring(loaded) .. ", " .. tostring(loadedMsg))
  assert(roomExists(7), "loadMap did not load the binary map")
  local imported, importedMsg = loadMap(headlessMapDir .. "/headless.xml")
  assert(imported == true, "loadMap of an XML map answered " .. tostring(imported) .. ", " .. tostring(importedMsg))
  assert(roomExists(3) and not roomExists(7), "loadMap did not replace the map with the XML one")
  assert(loadMap(headlessMapDir .. "/nosuchmap.dat") == false, "loadMap of a missing binary map did not answer false")
  local missing, missingMsg = loadMap(headlessMapDir .. "/nosuchmap.xml")
  assert(missing == nil and missingMsg:find("was not found"), "loadMap of a missing XML map answered " .. tostring(missing) .. ", " .. tostring(missingMsg))
end)
headlessMapLoad = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapLoad"), qsl("ok"));
        // As in the GUI, the first load made the map widget, which the later ones then load into
        QCOMPARE(luaGlobalString(host, "headlessMapLoadOpens"), qsl("1"));
        QCOMPARE(host->getMapperTitle(), std::optional<QString>(qsl("Map - %1").arg(hostname)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Loading a map created a widget.");
        HostManager::self()->deleteHost(hostname);
    }
    // As the GUI answers before and after loadMap makes its mapper
    void test_playerRoomAndJsonMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Lua");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QTemporaryDir mapDir;
        QVERIFY(mapDir.isValid());

        // The refusal is outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
        const QString script = qsl("headlessMapDir = [[%1]]\nnoMapOpen = \"you haven't opened a map yet\"\n").arg(mapDir.path()) + qsl(R"lua(
headlessMapLua = 'not run'
local ok, err = pcall(function()
  local function answers(...) return select("#", ...), ... end
  local count, first, second = answers(getPlayerRoom())
  assert(count == 2 and first == nil and second == noMapOpen,
    "getPlayerRoom with no mapper answered " .. tostring(first) .. ", " .. tostring(second))

  local area = addAreaName("Headless Lua Area")
  local kept = createRoomID()
  addRoom(kept); setRoomArea(kept, area)
  local moved = createRoomID()
  addRoom(moved); setRoomArea(moved, area)
  assert(saveMap(headlessMapDir .. "/rooms.dat") == true, "saveMap did not save the map")
  assert(loadMap(headlessMapDir .. "/rooms.dat") == true, "loadMap did not load the map")

  count, first, second = answers(centerview(moved))
  assert(count == 1 and first == true, "centerview answered " .. tostring(first) .. ", " .. tostring(second))
  count, first, second = answers(getPlayerRoom())
  assert(count == 1 and first == moved, "getPlayerRoom after centerview answered " .. tostring(first) .. ", " .. tostring(second))

  count, first, second = answers(saveJsonMap(headlessMapDir .. "/rooms.json"))
  assert(count == 1 and first == true, "saveJsonMap answered " .. tostring(first) .. ", " .. tostring(second))
  deleteRoom(kept)
  count, first, second = answers(loadJsonMap(headlessMapDir .. "/rooms.json"))
  assert(count == 1 and first == true, "loadJsonMap answered " .. tostring(first) .. ", " .. tostring(second))
  assert(roomExists(kept), "loadJsonMap did not read back the map saveJsonMap wrote")
  count, first, second = answers(loadJsonMap(headlessMapDir .. "/nosuchmap.json"))
  assert(count == 2 and first == nil and tostring(second):find("could not open file", 1, true),
    "loadJsonMap of a missing file answered " .. tostring(first) .. ", " .. tostring(second))
end)
headlessMapLua = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapLua"), qsl("ok"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The map functions created a widget.");
    }

    // As the GUI answers with its mapper up and no secondary map view open
    void test_mapperSettingsAndViewsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Map-Config");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const int area = host->mpMap->mpRoomDB->addArea(qsl("Headless Config Area"));
        QVERIFY(host->mpMap->mpRoomDB->set2DMapZoom(area, 7.5));
        // As a profile saved with the 3D view showing loads
        host->mShow3DView = true;

        // The refusals are outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
#if defined(INCLUDE_3DMAPPER)
        const QString threeD = qsl("hide3d = {n = 1, true}\nshow3d = {n = 2, nil, \"there is no main window to show the 3D map view in\"}\n");
#else
        const QString threeD = qsl("hide3d = {n = 2, nil, \"'show3dMapView' isn't a valid configuration option\"}\nshow3d = hide3d\n");
#endif
        const QString script = qsl("headlessArea = %1\n").arg(area) + threeD + qsl(R"lua(
headlessMapConfig = 'not run'
local ok, err = pcall(function()
  local function check(label, want, ...)
    local got = {n = select("#", ...), ...}
    local same = got.n == want.n
    for i = 1, math.max(got.n, want.n) do
      same = same and got[i] == want[i]
    end
    assert(same, label .. " answered " .. got.n .. " values: " .. tostring(got[1]) .. ", " .. tostring(got[2]))
  end
  local function refused(message) return {n = 2, nil, message} end
  local function answered(value) return {n = 1, value} end

  check("openMapWidget", answered(true), openMapWidget())
  local room = createRoomID()
  addRoom(room); setRoomArea(room, headlessArea)
  local ids = getMapViewIds()
  assert(type(ids) == "table" and next(ids) == nil, "getMapViewIds answered " .. tostring(ids))
  check("closeAllMapViews", answered(0), closeAllMapViews())
  local function counted(...) return select("#", ...), ... end
  local selectionCount, selection = counted(getMapSelection())
  assert(selectionCount == 1 and type(selection) == "table" and next(selection) == nil, "getMapSelection answered " .. selectionCount .. " values: " .. tostring(selection))
  check("clearMapSelection", answered(false), clearMapSelection())
  check("closeMapView", refused("view 99 not found"), closeMapView(99))
  check("getMapViewInfo", refused("view 99 not found"), getMapViewInfo(99))
  check("centerview in a view", refused("view 99 not found"), centerview(room, 99))
  check("getMapZoom in a view", refused("view 99 not found"), getMapZoom(headlessArea, 99))
  check("setMapZoom in a view", refused("view 99 not found"), setMapZoom(5, headlessArea, 99))
  check("createMapView", answered(1), createMapView(headlessArea))
  check("closeMapView", answered(true), closeMapView(1))

  check("getMapZoom of an area", answered(7.5), getMapZoom(headlessArea))
  check("getMapZoom of no such area", refused("number 9999 is not a valid areaID"), getMapZoom(9999))
  check("setMapZoom of an area", answered(true), setMapZoom(12.25, headlessArea))
  check("getMapZoom after setMapZoom", answered(12.25), getMapZoom(headlessArea))
  check("setMapZoom too small", refused("zoom 2 is invalid, it must be at least 3"), setMapZoom(2, headlessArea))
  check("setMapZoom not finite", refused("zoom nan is invalid, it must be a finite number"), setMapZoom(0/0))
  check("setMapZoom of no such area", refused("number 9999 is not a valid areaID"), setMapZoom(5, 9999))
  check("getMapZoom after refusals", answered(12.25), getMapZoom(headlessArea))
  check("setDefaultAreaVisible", answered(true), setDefaultAreaVisible(false))

  check("setConfig mapRoomSize", answered(true), setConfig("mapRoomSize", 6))
  check("getConfig mapRoomSize", answered(6), getConfig("mapRoomSize"))
  check("setConfig mapRoomSize 0", refused("mapRoomSize must be at least 1, got 0"), setConfig("mapRoomSize", 0))
  check("setConfig mapExitSize", answered(true), setConfig("mapExitSize", 3.5))
  check("getConfig mapExitSize", answered(3.5), getConfig("mapExitSize"))
  for _, key in ipairs({"mapRoundRooms", "showRoomIdsOnMap", "mapShowGrid"}) do
    check("setConfig " .. key, answered(true), setConfig(key, true))
    check("getConfig " .. key, answered(true), getConfig(key))
  end
  check("setConfig mapShowRoomBorders", answered(true), setConfig("mapShowRoomBorders", false))
  check("getConfig mapShowRoomBorders", answered(false), getConfig("mapShowRoomBorders"))
  check("setConfig showMapInfo", answered(true), setConfig("showMapInfo", "Headless"))
  check("setConfig mapInfoColor", answered(true), setConfig("mapInfoColor", {1, 2, 3}))
  local color = getConfig("mapInfoColor")
  assert(color[1] == 1 and color[2] == 2 and color[3] == 3 and color[4] == 255, "getConfig mapInfoColor did not read back the color set")
  check("setConfig mapInfoColor 'x'", refused("mapInfoColor requires a table {r, g, b} or {r, g, b, a}"), setConfig("mapInfoColor", "x"))
  check("getConfig showUpperLowerLevels default", answered(true), getConfig("showUpperLowerLevels"))
  check("setConfig showUpperLowerLevels", answered(true), setConfig("showUpperLowerLevels", false))
  check("getConfig showUpperLowerLevels", answered(false), getConfig("showUpperLowerLevels"))
  -- The null view's mapper has no 3D view, so it can only hide one
  check("getConfig show3dMapView", answered(false), getConfig("show3dMapView"))
  check("setConfig show3dMapView false", hide3d, setConfig("show3dMapView", false))
  check("getConfig show3dMapView hidden", answered(false), getConfig("show3dMapView"))
  check("setConfig show3dMapView true", show3d, setConfig("show3dMapView", true))
end)
headlessMapConfig = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapConfig"), qsl("ok"));
        QVERIFY2(!host->mpMap->getDefaultAreaShown(), "setDefaultAreaVisible(false) did not hide the default area.");
        QVERIFY2(host->mMapInfoContributors.contains(qsl("Headless")), "setConfig showMapInfo did not add the contributor.");
#if defined(INCLUDE_3DMAPPER)
        QVERIFY2(!host->mShow3DView, "Hiding the 3D view was not kept for the profile to save.");
#endif
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The map functions created a widget.");
    }

    // The answers are those of the same Lua chunk run in the GUI self-test profile, which has no mapper until loadMap
    // makes one; closing the map widget then only hides it
    void test_mapOpenContract()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        const QString hostname = qsl("Test-Headless-Host-Map-Open");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->getLuaInterpreter()->loadGlobal();
        // With no view, pumpEvents() takes Mudlet for shutting down and pumps nothing
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State*) -> int {
            for (int pass = 0; pass < 10; ++pass) {
                QCoreApplication::processEvents();
            }
            return 0;
        });
        const QString chunk = qsl(R"lua(
headlessMapOpen = "not run"
local ok, err = pcall(function()
  local profile = getProfileName():gsub("%p", "%%%0")
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = (select(i, ...))
      if type(value) == "table" then
        local keys = {}
        for k in pairs(value) do keys[#keys + 1] = tostring(k) end
        table.sort(keys)
        value = "{" .. table.concat(keys, ",") .. "}"
      end
      values[i] = tostring(value):gsub(profile, "<profile>")
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local area = addAreaName("Headless Map Open Area")
  local room = createRoomID()
  addRoom(room)
  setRoomArea(room, area)
  setRoomCoordinates(room, 0, 0, 0)
  local home = getMudletHomeDir()
  local jsonPath = home .. "/map open.json"
  local datPath = home .. "/map open.dat"
  local imagePath = home .. "/map open.png"
  local function calls(suffix)
    step("getPlayerRoom" .. suffix, getPlayerRoom)
    step("centerview" .. suffix, centerview, room)
    step("getPlayerRoom.centred" .. suffix, getPlayerRoom)
    step("saveJsonMap" .. suffix, saveJsonMap, jsonPath)
    step("loadJsonMap" .. suffix, loadJsonMap, jsonPath)
    step("setDefaultAreaVisible" .. suffix, setDefaultAreaVisible, true)
    step("getMapZoom.area" .. suffix, getMapZoom, area)
    step("setMapZoom.area" .. suffix, setMapZoom, 6, area)
    step("setMapZoom.badArea" .. suffix, setMapZoom, 6, 9999)
    step("setMapZoom.small" .. suffix, setMapZoom, 1, area)
    step("getMapSelection" .. suffix, getMapSelection)
    step("clearMapSelection" .. suffix, clearMapSelection)
    for _, key in ipairs({"mapRoomSize", "mapExitSize", "mapRoundRooms", "showRoomIdsOnMap", "mapShowRoomBorders", "mapShowGrid", "showUpperLowerLevels"}) do
      step("setConfig." .. key .. suffix, setConfig, key, getConfig(key))
    end
    step("setConfig.showMapInfo" .. suffix, setConfig, "showMapInfo", "Probe")
    step("setConfig.hideMapInfo" .. suffix, setConfig, "hideMapInfo", "Probe")
    step("setConfig.mapInfoColor" .. suffix, setConfig, "mapInfoColor", {1, 2, 3})
    step("getMapViewIds" .. suffix, getMapViewIds)
  end
  step("getMapZoom.none", getMapZoom)
  step("setMapZoom.none", setMapZoom, 5)
  calls(".none")
  -- Only without a mapper: drawing the area takes T2DMap, which a profile with no main window lacks
  step("exportAreaImage.none", exportAreaImage, area, imagePath)
  step("saveMap.none", saveMap, datPath)
  step("getMapZoom.none2", getMapZoom)
  step("loadMap", loadMap, datPath)
  pumpEvents(300)
  calls(".loaded")
  step("closeMapWidget", closeMapWidget)
  calls(".closed")
  os.remove(jsonPath)
  os.remove(datPath)
  os.remove(imagePath)
  headlessMapOpenAnswers = table.concat(answers, "\n")
end)
headlessMapOpen = ok and "ok" or tostring(err)
)lua");
        const QStringList answers{qsl("getMapZoom.none=[2]nil:no active mapper"),
                                  qsl("setMapZoom.none=[2]nil:no active mapper"),
                                  qsl("getPlayerRoom.none=[2]nil:you haven't opened a map yet"),
                                  qsl("centerview.none=[2]nil:you haven't opened a map yet"),
                                  qsl("getPlayerRoom.centred.none=[2]nil:you haven't opened a map yet"),
                                  qsl("saveJsonMap.none=[2]nil:no map present or loaded"),
                                  qsl("loadJsonMap.none=[2]nil:no map present or loaded"),
                                  qsl("setDefaultAreaVisible.none=[1]false"),
                                  qsl("getMapZoom.area.none=[2]nil:no active mapper"),
                                  qsl("setMapZoom.area.none=[2]nil:no active mapper"),
                                  qsl("setMapZoom.badArea.none=[2]nil:no active mapper"),
                                  qsl("setMapZoom.small.none=[2]nil:no active mapper"),
                                  qsl("getMapSelection.none=[2]nil:no map present or loaded"),
                                  qsl("clearMapSelection.none=[2]nil:no map present or loaded"),
                                  qsl("setConfig.mapRoomSize.none=[1]true"),
                                  qsl("setConfig.mapExitSize.none=[1]true"),
                                  qsl("setConfig.mapRoundRooms.none=[1]true"),
                                  qsl("setConfig.showRoomIdsOnMap.none=[1]true"),
                                  qsl("setConfig.mapShowRoomBorders.none=[1]true"),
                                  qsl("setConfig.mapShowGrid.none=[1]true"),
                                  qsl("setConfig.showUpperLowerLevels.none=[1]true"),
                                  qsl("setConfig.showMapInfo.none=[1]true"),
                                  qsl("setConfig.hideMapInfo.none=[1]true"),
                                  qsl("setConfig.mapInfoColor.none=[1]true"),
                                  qsl("getMapViewIds.none=[1]{}"),
                                  qsl("exportAreaImage.none=[2]nil:map needs to be open"),
                                  qsl("saveMap.none=[1]true"),
                                  qsl("getMapZoom.none2=[2]nil:no active mapper"),
                                  qsl("loadMap=[1]true"),
                                  qsl("getPlayerRoom.loaded=[2]nil:the player does not have a valid roomID set"),
                                  qsl("centerview.loaded=[1]true"),
                                  qsl("getPlayerRoom.centred.loaded=[1]1"),
                                  qsl("saveJsonMap.loaded=[1]true"),
                                  qsl("loadJsonMap.loaded=[1]true"),
                                  qsl("setDefaultAreaVisible.loaded=[1]true"),
                                  qsl("getMapZoom.area.loaded=[1]20"),
                                  qsl("setMapZoom.area.loaded=[1]true"),
                                  qsl("setMapZoom.badArea.loaded=[2]nil:number 9999 is not a valid areaID"),
                                  qsl("setMapZoom.small.loaded=[2]nil:zoom 1 is invalid, it must be at least 3"),
                                  qsl("getMapSelection.loaded=[1]{}"),
                                  qsl("clearMapSelection.loaded=[1]false"),
                                  qsl("setConfig.mapRoomSize.loaded=[1]true"),
                                  qsl("setConfig.mapExitSize.loaded=[1]true"),
                                  qsl("setConfig.mapRoundRooms.loaded=[1]true"),
                                  qsl("setConfig.showRoomIdsOnMap.loaded=[1]true"),
                                  qsl("setConfig.mapShowRoomBorders.loaded=[1]true"),
                                  qsl("setConfig.mapShowGrid.loaded=[1]true"),
                                  qsl("setConfig.showUpperLowerLevels.loaded=[1]true"),
                                  qsl("setConfig.showMapInfo.loaded=[1]true"),
                                  qsl("setConfig.hideMapInfo.loaded=[1]true"),
                                  qsl("setConfig.mapInfoColor.loaded=[1]true"),
                                  qsl("getMapViewIds.loaded=[1]{}"),
                                  qsl("closeMapWidget=[1]true"),
                                  qsl("getPlayerRoom.closed=[1]1"),
                                  qsl("centerview.closed=[1]true"),
                                  qsl("getPlayerRoom.centred.closed=[1]1"),
                                  qsl("saveJsonMap.closed=[1]true"),
                                  qsl("loadJsonMap.closed=[1]true"),
                                  qsl("setDefaultAreaVisible.closed=[1]true"),
                                  qsl("getMapZoom.area.closed=[1]6"),
                                  qsl("setMapZoom.area.closed=[1]true"),
                                  qsl("setMapZoom.badArea.closed=[2]nil:number 9999 is not a valid areaID"),
                                  qsl("setMapZoom.small.closed=[2]nil:zoom 1 is invalid, it must be at least 3"),
                                  qsl("getMapSelection.closed=[1]{}"),
                                  qsl("clearMapSelection.closed=[1]false"),
                                  qsl("setConfig.mapRoomSize.closed=[1]true"),
                                  qsl("setConfig.mapExitSize.closed=[1]true"),
                                  qsl("setConfig.mapRoundRooms.closed=[1]true"),
                                  qsl("setConfig.showRoomIdsOnMap.closed=[1]true"),
                                  qsl("setConfig.mapShowRoomBorders.closed=[1]true"),
                                  qsl("setConfig.mapShowGrid.closed=[1]true"),
                                  qsl("setConfig.showUpperLowerLevels.closed=[1]true"),
                                  qsl("setConfig.showMapInfo.closed=[1]true"),
                                  qsl("setConfig.hideMapInfo.closed=[1]true"),
                                  qsl("setConfig.mapInfoColor.closed=[1]true"),
                                  qsl("getMapViewIds.closed=[1]{}")};

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(chunk), "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapOpen"), qsl("ok"));
        const QStringList actual = luaGlobalString(host, "headlessMapOpenAnswers").split(QLatin1Char('\n'));
        QStringList different;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                different << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(different.isEmpty(), qPrintable(different.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the map created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua chunk run in the GUI self-test profile reopened with the map it saved:
    // opening a profile loads its map, but only openMapWidget makes the mapper
    void test_reopenedProfileMapOpenContract()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        const QString makerHostname = qsl("Test-Headless-Host-Reopen-Map-Maker");
        QVERIFY2(HostManager::self()->addHost(makerHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        const QByteArray map = binaryMapOf(HostManager::self()->getHost(makerHostname), 9);
        HostManager::self()->deleteHost(makerHostname);

        const QString hostname = qsl("Test-Headless-Host-Reopen-Map");
        const QString mapFolder = MudletApp::getMudletPath(enums::profileMapsPath, hostname);
        QVERIFY(QDir().mkpath(mapFolder));
        QFile mapFile(qsl("%1/headless.dat").arg(mapFolder));
        QVERIFY(mapFile.open(QIODevice::WriteOnly));
        mapFile.write(map);
        mapFile.close();
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open.");
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State*) -> int {
            for (int pass = 0; pass < 10; ++pass) {
                QCoreApplication::processEvents();
            }
            return 0;
        });
        const QString chunk = qsl(R"lua(
headlessMapOpen = "not run"
local ok, err = pcall(function()
  local profile = getProfileName():gsub("%p", "%%%0")
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = (select(i, ...))
      if type(value) == "table" then
        local keys = {}
        for k in pairs(value) do keys[#keys + 1] = tostring(k) end
        table.sort(keys)
        value = "{" .. table.concat(keys, ",") .. "}"
      end
      values[i] = tostring(value):gsub(profile, "<profile>")
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local room = 9
  local area = getRoomArea(room)
  local home = getMudletHomeDir()
  local jsonPath = home .. "/map reopen.json"
  local imagePath = home .. "/map reopen.png"
  local function calls(suffix)
    step("getPlayerRoom" .. suffix, getPlayerRoom)
    step("centerview" .. suffix, centerview, room)
    step("getPlayerRoom.centred" .. suffix, getPlayerRoom)
    step("saveJsonMap" .. suffix, saveJsonMap, jsonPath)
    step("loadJsonMap" .. suffix, loadJsonMap, jsonPath)
    step("getMapZoom.area" .. suffix, getMapZoom, area)
    step("setMapZoom.area" .. suffix, setMapZoom, 6, area)
    step("getMapSelection" .. suffix, getMapSelection)
    step("setConfig.mapRoomSize" .. suffix, setConfig, "mapRoomSize", getConfig("mapRoomSize"))
    step("exportAreaImage" .. suffix, exportAreaImage, area, imagePath)
    step("getConfig.show3dMapView" .. suffix, getConfig, "show3dMapView")
    step("setConfig.show3dMapView.false" .. suffix, setConfig, "show3dMapView", false)
  end
  step("rooms", function() return table.size(getRooms()) .. ":" .. tostring(roomExists(9)) end)
  calls(".reopened")
  step("openMapWidget", openMapWidget)
  pumpEvents(300)
  calls(".opened")
  os.remove(jsonPath)
  os.remove(imagePath)
  headlessMapOpenAnswers = table.concat(answers, "\n")
end)
headlessMapOpen = ok and "ok" or tostring(err)
)lua");
#if defined(INCLUDE_3DMAPPER)
        const QString hide3dReopened = qsl("setConfig.show3dMapView.false.reopened=[1]true");
        const QString hide3dOpened = qsl("setConfig.show3dMapView.false.opened=[1]true");
#else
        const QString hide3dReopened = qsl("setConfig.show3dMapView.false.reopened=[2]nil:'show3dMapView' isn't a valid configuration option");
        const QString hide3dOpened = qsl("setConfig.show3dMapView.false.opened=[2]nil:'show3dMapView' isn't a valid configuration option");
#endif
        const QStringList answers{qsl("rooms=[1]1:true"),
                                  qsl("getPlayerRoom.reopened=[2]nil:you haven't opened a map yet"),
                                  qsl("centerview.reopened=[2]nil:you haven't opened a map yet"),
                                  qsl("getPlayerRoom.centred.reopened=[2]nil:you haven't opened a map yet"),
                                  qsl("saveJsonMap.reopened=[2]nil:no map present or loaded"),
                                  qsl("loadJsonMap.reopened=[2]nil:no map present or loaded"),
                                  qsl("getMapZoom.area.reopened=[2]nil:no active mapper"),
                                  qsl("setMapZoom.area.reopened=[2]nil:no active mapper"),
                                  qsl("getMapSelection.reopened=[2]nil:no map present or loaded"),
                                  qsl("setConfig.mapRoomSize.reopened=[1]true"),
                                  qsl("exportAreaImage.reopened=[2]nil:map needs to be open"),
                                  qsl("getConfig.show3dMapView.reopened=[1]false"),
                                  hide3dReopened,
                                  qsl("openMapWidget=[1]true"),
                                  qsl("getPlayerRoom.opened=[2]nil:the player does not have a valid roomID set"),
                                  qsl("centerview.opened=[1]true"),
                                  qsl("getPlayerRoom.centred.opened=[1]9"),
                                  qsl("saveJsonMap.opened=[1]true"),
                                  qsl("loadJsonMap.opened=[1]true"),
                                  qsl("getMapZoom.area.opened=[1]20"),
                                  qsl("setMapZoom.area.opened=[1]true"),
                                  qsl("getMapSelection.opened=[1]{}"),
                                  qsl("setConfig.mapRoomSize.opened=[1]true"),
                                  // The GUI draws the image, with the T2DMap the null view lacks
                                  qsl("exportAreaImage.opened=[2]false:there is no main window to draw the map in"),
                                  qsl("getConfig.show3dMapView.opened=[1]false"),
                                  hide3dOpened};

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(chunk), "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapOpen"), qsl("ok"));
        const QStringList actual = luaGlobalString(host, "headlessMapOpenAnswers").split(QLatin1Char('\n'));
        QStringList different;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                different << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(different.isEmpty(), qPrintable(different.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the map created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    void test_secondaryMapViewsContract()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        const QString hostname = qsl("Test-Headless-Host-Map-Views");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->getLuaInterpreter()->loadGlobal();
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State*) -> int {
            for (int pass = 0; pass < 10; ++pass) {
                QCoreApplication::processEvents();
            }
            return 0;
        });
        const QString chunk = qsl(R"lua(
headlessMapOpen = "not run"
local ok, err = pcall(function()
  local profile = getProfileName():gsub("%p", "%%%0")
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = (select(i, ...))
      if type(value) == "table" then
        local keys = {}
        for k in pairs(value) do keys[#keys + 1] = tostring(k) end
        table.sort(keys)
        value = "{" .. table.concat(keys, ",") .. "}"
      end
      values[i] = tostring(value):gsub(profile, "<profile>")
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function info(view)
    local i, why = getMapViewInfo(view)
    if type(i) ~= "table" then return tostring(i) .. ":" .. tostring(why) end
    return string.format("area=%s room=%s zoom=%s z=%s", tostring(i.areaId), tostring(i.centeredRoomId), tostring(i.zoom), tostring(i.zLevel))
  end
  local events = {}
  local handler = registerAnonymousEventHandler("sysMapAreaChanged", function(_, new, old)
    events[#events + 1] = tostring(new) .. "<" .. tostring(old)
  end)
  local function flushEvents(label)
    answers[#answers + 1] = "events." .. label .. "=" .. table.concat(events, ",")
    events = {}
  end
  local a = addAreaName("Views A")
  local b = addAreaName("Views B")
  local c = addAreaName("Views C")
  local function ids()
    return table.concat(getMapViewIds(), ",")
  end
  local function room(id, area, x, y, z)
    addRoom(id)
    setRoomArea(id, area)
    setRoomCoordinates(id, x, y, z)
  end
  room(101, a, 0, 0, 0)
  room(102, a, 1, 0, 0)
  room(103, b, 0, 0, 2)
  room(104, b, 5, 5, 2)
  room(105, b, 0, 0, -1)
  step("ids.start", ids)
  step("create.bad", createMapView, 9999)
  step("create.badType", createMapView, "x")
  local _, v1 = pcall(createMapView)
  answers[#answers + 1] = "create.none=" .. tostring(v1)
  step("info.v1", info, v1)
  flushEvents("v1")
  pumpEvents(300)
  step("info.v1.pumped", info, v1)
  step("zoom.v1", setMapZoom, 7, 0, v1)
  step("getzoom.v1", getMapZoom, 0, v1)
  local _, v2 = pcall(createMapView, a)
  answers[#answers + 1] = "create.a=" .. tostring(v2)
  step("info.v2", info, v2)
  flushEvents("v2")
  local _, v3 = pcall(createMapView, b)
  answers[#answers + 1] = "create.b=" .. tostring(v3)
  step("info.v3", info, v3)
  flushEvents("v3")
  local _, v4 = pcall(createMapView, -1)
  answers[#answers + 1] = "create.default=" .. tostring(v4)
  step("info.v4", info, v4)
  local _, v5 = pcall(createMapView, c)
  answers[#answers + 1] = "create.c=" .. tostring(v5)
  step("info.v5", info, v5)
  flushEvents("v4v5")
  pumpEvents(300)
  step("ids", ids)
  step("info.v2.pumped", info, v2)
  step("info.v3.pumped", info, v3)
  step("info.v5.pumped", info, v5)
  flushEvents("pumped")
  step("center.v2", centerview, 103, v2)
  step("info.v2.centred", info, v2)
  step("center.v1", centerview, 105, v1)
  step("info.v1.centred", info, v1)
  step("center.missingView", centerview, 101, 999)
  step("center.missingRoom", centerview, 999, v2)
  flushEvents("centred")
  step("zoom.small", setMapZoom, 1, 0, v2)
  step("zoom.v2", setMapZoom, 8, 0, v2)
  step("info.v2.zoomed", info, v2)
  step("getzoom.v2", getMapZoom, 0, v2)
  step("zoom.missingView", setMapZoom, 8, 0, 999)
  step("getzoom.missingView", getMapZoom, 0, 999)
  pumpEvents(300)
  step("info.v2.zoomedPumped", info, v2)
  step("info.v3.zoomedPumped", info, v3)
  step("zoom.v3same", setMapZoom, 8, 0, v3)
  step("info.missing", info, 999)
  step("info.badType", getMapViewInfo, "x")
  step("deleteArea.b", deleteArea, b)
  step("info.v1.deleted", info, v1)
  step("info.v2.deleted", info, v2)
  step("info.v3.deleted", info, v3)
  flushEvents("deleted")
  pumpEvents(300)
  step("info.v2.deletedPumped", info, v2)
  step("close.v1", closeMapView, v1)
  step("close.v1again", closeMapView, v1)
  step("close.badType", closeMapView, "x")
  step("ids.closed", ids)
  step("info.v1.closed", info, v1)
  step("closeAll", closeAllMapViews)
  step("ids.allClosed", ids)
  step("closeAll.none", closeAllMapViews)
  pumpEvents(300)
  local _, v6 = pcall(createMapView, a)
  answers[#answers + 1] = "create.afterClose=" .. tostring(v6)
  pumpEvents(300)
  step("info.v6", info, v6)
  flushEvents("v6")
  step("openMapWidget", openMapWidget)
  step("centerview.player", centerview, 102)
  pumpEvents(300)
  flushEvents("player")
  local _, v7 = pcall(createMapView, a)
  step("info.v7", info, v7)
  local _, v8 = pcall(createMapView, c)
  step("info.v8", info, v8)
  flushEvents("v7v8")
  step("zoom.v7", setMapZoom, 9, 0, v7)
  step("info.v7.zoomed", info, v7)
  step("getzoom.v7", getMapZoom, 0, v7)
  pumpEvents(300)
  step("info.v7.zoomedPumped", info, v7)
  step("info.v6.zoomedPumped", info, v6)
  step("getzoom.a", getMapZoom, a)
  step("zoom.v8", setMapZoom, 4, 0, v8)
  pumpEvents(300)
  step("info.v8.zoomedPumped", info, v8)
  step("setDefaultAreaVisible", setDefaultAreaVisible, false)
  step("deleteArea.c", deleteArea, c)
  pumpEvents(300)
  step("info.v8.deleted", info, v8)
  flushEvents("end")
  step("closeAll.end", closeAllMapViews)
  for id in pairs(getRooms()) do deleteRoom(id) end
  pumpEvents(300)
  flushEvents("roomsDeleted")
  local d = addAreaName("Views D")
  local _, v9 = pcall(createMapView, d)
  local _, v10 = pcall(createMapView, d)
  flushEvents("v9v10")
  step("zoom.v9", setMapZoom, 6, 0, v9)
  pumpEvents(300)
  step("info.v9.noRooms", info, v9)
  step("info.v10.noRooms", info, v10)
  room(106, d, 0, 0, 0)
  pumpEvents(300)
  step("info.v9.roomAdded", info, v9)
  step("info.v10.roomAdded", info, v10)
  step("zoom.mapper", setMapZoom, 7, d)
  pumpEvents(300)
  step("info.v9.mapperZoomed", info, v9)
  step("info.v10.mapperZoomed", info, v10)
  local e = addAreaName("Views E")
  room(107, e, 0, 0, 0)
  local _, v11 = pcall(createMapView, e)
  local _, v12 = pcall(createMapView, e)
  flushEvents("v11v12")
  local made, closed
  local meddler = registerAnonymousEventHandler("sysMapAreaChanged", function()
    if made == nil then
      closed = described(pcall(closeMapView, v12))
      made = createMapView()
    end
  end)
  step("deleteArea.e", deleteArea, e)
  killAnonymousEventHandler(meddler)
  answers[#answers + 1] = "meddle.close=" .. tostring(closed)
  answers[#answers + 1] = "meddle.create=" .. tostring(made)
  step("ids.meddled", ids)
  step("info.v11.meddled", info, v11)
  step("info.made", info, made)
  flushEvents("meddled")
  local others = {}
  for id in pairs(getAreaTableSwap()) do
    if id ~= -1 and id ~= d then others[#others + 1] = id end
  end
  table.sort(others)
  for _, id in ipairs(others) do deleteArea(id) end
  step("deleteArea.last", deleteArea, d)
  step("info.v9.allDeleted", info, v9)
  step("info.v10.allDeleted", info, v10)
  step("info.v11.allDeleted", info, v11)
  step("info.made.allDeleted", info, made)
  flushEvents("allDeleted")
  step("closeAll.final", closeAllMapViews)
  killAnonymousEventHandler(handler)
  headlessMapOpenAnswers = table.concat(answers, "\n")
end)
headlessMapOpen = ok and "ok" or tostring(err)
)lua");
        const QStringList answers{qsl("ids.start=[1]"),
                                  qsl("create.bad=[2]nil:area 9999 does not exist"),
                                  qsl("create.badType=raised:createMapView: bad argument #1 type (area id as number is optional, got string!)"),
                                  qsl("create.none=1"),
                                  qsl("info.v1=[1]area=0 room=0 zoom=20 z=0"),
                                  qsl("events.v1="),
                                  qsl("info.v1.pumped=[1]area=0 room=0 zoom=20 z=0"),
                                  qsl("zoom.v1=[2]nil:number 0 is not a valid areaID"),
                                  qsl("getzoom.v1=[1]20"),
                                  qsl("create.a=2"),
                                  qsl("info.v2=[1]area=1 room=0 zoom=20 z=0"),
                                  qsl("events.v2=1<0"),
                                  qsl("create.b=3"),
                                  qsl("info.v3=[1]area=2 room=0 zoom=20 z=2"),
                                  qsl("events.v3=2<0"),
                                  qsl("create.default=4"),
                                  qsl("info.v4=[1]area=0 room=0 zoom=20 z=0"),
                                  qsl("create.c=5"),
                                  qsl("info.v5=[1]area=3 room=0 zoom=20 z=0"),
                                  qsl("events.v4v5=3<0"),
                                  qsl("ids=[1]1,2,3,4,5"),
                                  qsl("info.v2.pumped=[1]area=1 room=0 zoom=20 z=0"),
                                  qsl("info.v3.pumped=[1]area=2 room=0 zoom=20 z=2"),
                                  qsl("info.v5.pumped=[1]area=3 room=0 zoom=20 z=0"),
                                  qsl("events.pumped="),
                                  qsl("center.v2=[1]true"),
                                  qsl("info.v2.centred=[1]area=2 room=103 zoom=20 z=2"),
                                  qsl("center.v1=[1]true"),
                                  qsl("info.v1.centred=[1]area=2 room=105 zoom=20 z=-1"),
                                  qsl("center.missingView=[2]nil:view 999 not found"),
                                  qsl("center.missingRoom=[2]nil:number 999 is not a valid roomID"),
                                  qsl("events.centred="),
                                  qsl("zoom.small=[2]nil:zoom 1 is invalid, it must be at least 3"),
                                  qsl("zoom.v2=[1]true"),
                                  qsl("info.v2.zoomed=[1]area=2 room=103 zoom=20 z=2"),
                                  qsl("getzoom.v2=[1]20"),
                                  qsl("zoom.missingView=[2]nil:view 999 not found"),
                                  qsl("getzoom.missingView=[2]nil:view 999 not found"),
                                  qsl("info.v2.zoomedPumped=[1]area=2 room=103 zoom=8 z=2"),
                                  qsl("info.v3.zoomedPumped=[1]area=2 room=0 zoom=8 z=2"),
                                  qsl("zoom.v3same=[1]true"),
                                  qsl("info.missing=[1]nil:view 999 not found"),
                                  qsl("info.badType=raised:getMapViewInfo: bad argument #1 type (view id as number expected, got string!)"),
                                  qsl("deleteArea.b=[1]true"),
                                  qsl("info.v1.deleted=[1]area=-1 room=105 zoom=20 z=0"),
                                  qsl("info.v2.deleted=[1]area=-1 room=103 zoom=20 z=0"),
                                  qsl("info.v3.deleted=[1]area=-1 room=0 zoom=20 z=0"),
                                  qsl("events.deleted=-1<2,-1<2,-1<2"),
                                  qsl("info.v2.deletedPumped=[1]area=-1 room=103 zoom=20 z=0"),
                                  qsl("close.v1=[1]true"),
                                  qsl("close.v1again=[2]nil:view 1 not found"),
                                  qsl("close.badType=raised:closeMapView: bad argument #1 type (view id as number expected, got string!)"),
                                  qsl("ids.closed=[1]2,3,4,5"),
                                  qsl("info.v1.closed=[1]nil:view 1 not found"),
                                  qsl("closeAll=[1]4"),
                                  qsl("ids.allClosed=[1]"),
                                  qsl("closeAll.none=[1]0"),
                                  qsl("create.afterClose=6"),
                                  qsl("info.v6=[1]area=1 room=0 zoom=20 z=0"),
                                  qsl("events.v6=1<0"),
                                  qsl("openMapWidget=[1]true"),
                                  qsl("centerview.player=[1]true"),
                                  qsl("events.player=1<-2"),
                                  qsl("info.v7=[1]area=1 room=0 zoom=20 z=0"),
                                  qsl("info.v8=[1]area=3 room=0 zoom=20 z=0"),
                                  qsl("events.v7v8=1<0,3<0"),
                                  qsl("zoom.v7=[1]true"),
                                  qsl("info.v7.zoomed=[1]area=1 room=0 zoom=20 z=0"),
                                  qsl("getzoom.v7=[1]20"),
                                  qsl("info.v7.zoomedPumped=[1]area=1 room=0 zoom=9 z=0"),
                                  qsl("info.v6.zoomedPumped=[1]area=1 room=0 zoom=9 z=0"),
                                  qsl("getzoom.a=[1]9"),
                                  qsl("zoom.v8=[1]true"),
                                  qsl("info.v8.zoomedPumped=[1]area=3 room=0 zoom=4 z=0"),
                                  qsl("setDefaultAreaVisible=[1]true"),
                                  qsl("deleteArea.c=[1]true"),
                                  qsl("info.v8.deleted=[1]area=1 room=0 zoom=9 z=0"),
                                  qsl("events.end=1<3"),
                                  qsl("closeAll.end=[1]3"),
                                  qsl("events.roomsDeleted="),
                                  qsl("events.v9v10=4<0,4<0"),
                                  qsl("zoom.v9=[1]true"),
                                  qsl("info.v9.noRooms=[1]area=4 room=0 zoom=20 z=0"),
                                  qsl("info.v10.noRooms=[1]area=4 room=0 zoom=20 z=0"),
                                  qsl("info.v9.roomAdded=[1]area=4 room=0 zoom=6 z=0"),
                                  qsl("info.v10.roomAdded=[1]area=4 room=0 zoom=6 z=0"),
                                  qsl("zoom.mapper=[1]true"),
                                  qsl("info.v9.mapperZoomed=[1]area=4 room=0 zoom=7 z=0"),
                                  qsl("info.v10.mapperZoomed=[1]area=4 room=0 zoom=7 z=0"),
                                  qsl("events.v11v12=4<1,5<0,5<0"),
                                  qsl("deleteArea.e=[1]true"),
                                  qsl("meddle.close=[1]true"),
                                  qsl("meddle.create=13"),
                                  qsl("ids.meddled=[1]9,10,11,13"),
                                  qsl("info.v11.meddled=[1]area=1 room=0 zoom=9 z=0"),
                                  qsl("info.made=[1]area=0 room=0 zoom=20 z=0"),
                                  qsl("events.meddled=1<5"),
                                  qsl("deleteArea.last=[1]true"),
                                  qsl("info.v9.allDeleted=[1]area=-1 room=0 zoom=20 z=0"),
                                  qsl("info.v10.allDeleted=[1]area=-1 room=0 zoom=20 z=0"),
                                  qsl("info.v11.allDeleted=[1]area=-1 room=0 zoom=20 z=0"),
                                  qsl("info.made.allDeleted=[1]area=0 room=0 zoom=20 z=0"),
                                  qsl("events.allDeleted=4<1,-1<4,-1<4,-1<4"),
                                  qsl("closeAll.final=[1]4")};

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(chunk), "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapOpen"), qsl("ok"));
        const QStringList actual = luaGlobalString(host, "headlessMapOpenAnswers").split(QLatin1Char('\n'));
        QStringList different;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                different << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(different.isEmpty(), qPrintable(different.join(QChar::LineFeed)));

        // TMapViewManager's dock for the view, which no Lua call reads
        const auto [viewId, refusal] = host->createMapView(0);
        QCOMPARE(viewId, 14);
        const auto* views = dynamic_cast<TNullMapViews*>(host->mpMap->mapViewsFrontend());
        QVERIFY2(views, "The profile has no null secondary views.");
        const TNullSecondaryMapView* view = views->record(viewId);
        QVERIFY(view);
        QCOMPARE(view->mDock.title, qsl("Map View 14 - %1").arg(hostname));
        QVERIFY(view->mDock.floating);
        QCOMPARE(view->mDock.size, QSize(400, 500));
        QVERIFY(view->mDock.shown);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the map views created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // An embedder closes a profile by deleting it from the pool, and quits by destroying the pool
    void test_exitEventIsRaisedOnceAsTheProfileGoes()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = qsl("%1/exits.txt").arg(recordDir.path());
        // The record has to outlive the profile, so it goes to a file, opened binary so Windows writes no \r. A
        // reset is refused under the handler, as in the GUI, since one reached while the handler pumps events would
        // close the Lua state it runs on.
        const QString handler = qsl(R"lua(
function onHeadlessExit(event)
  local file = io.open([[%1]], "ab")
  file:write(getProfileName(), " ", event, " reset:", tostring(resetProfile()), "\n")
  file:close()
end
)lua")
                                        .arg(recordPath);
        const auto recordExits = [&handler](Host* host) {
            host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onHeadlessExit"));
            return host->getLuaInterpreter()->compileAndExecuteScript(handler);
        };
        const auto recorded = [&recordPath]() {
            QFile file(recordPath);
            return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
        };

        const QString deletedHostname = qsl("Test-Headless-Host-Exit-Deleted");
        QVERIFY2(HostManager::self()->addHost(deletedHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(recordExits(HostManager::self()->getHost(deletedHostname)), "The Lua chunk did not run.");
        QVERIFY2(recorded().isEmpty(), "sysExitEvent was raised before the profile was closed.");
        HostManager::self()->deleteHost(deletedHostname);
        QCOMPARE(recorded(), qsl("%1 sysExitEvent reset:false\n").arg(deletedHostname));

        // A second pool, as this test's own has to last until cleanupTestCase()
        const QString quitHostname = qsl("Test-Headless-Host-Exit-Quit");
        auto pool = std::make_unique<HostManager>();
        QVERIFY2(pool->addHost(quitHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(recordExits(pool->getHost(quitHostname)), "The Lua chunk did not run.");
        pool.reset();
        QCOMPARE(recorded(), qsl("%1 sysExitEvent reset:false\n%2 sysExitEvent reset:false\n").arg(deletedHostname, quitHostname));
    }

    // An exit handler run as its pool is destroyed adds no profile, which the pool would drop without closing it
    void test_headlessPoolEndingAddsNoProfile()
    {
        const QString hostname = qsl("Test-Headless-Host-Pool-End-Adds-None");
        const QString addedHostname = qsl("Test-Headless-Host-Pool-End-Added");
        QStringList record;
        // A second pool, as in test_exitEventIsRaisedOnceAsTheProfileGoes()
        auto pool = std::make_unique<HostManager>();
        QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = pool->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_pushlightuserdata(L, pool.get());
        lua_pushlightuserdata(L, &record);
        lua_pushcclosure(
                L,
                [](lua_State* L) -> int {
                    auto pPool = static_cast<HostManager*>(lua_touserdata(L, lua_upvalueindex(1)));
                    auto pRecord = static_cast<QStringList*>(lua_touserdata(L, lua_upvalueindex(2)));
                    pRecord->append(qsl("sysExitEvent"));
                    pRecord->append(pPool->addHost(QString::fromUtf8(lua_tostring(L, 1)), QString(), QString(), QString()) ? qsl("added") : qsl("refused"));
                    return 0;
                },
                2);
        lua_setglobal(L, "addProfileAsPoolEnds");
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onPoolEndExit"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onPoolEndExit() addProfileAsPoolEnds([[%1]]) end").arg(addedHostname)));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(qsl("HostManager::addHost() - not adding \"%1\" as the pool is being destroyed").arg(addedHostname)));
        pool.reset();
        QCOMPARE(record, QStringList({qsl("sysExitEvent"), qsl("refused")}));
        QFile::remove(headlessLayoutPath());
    }

    void test_openProfileLoadsAndStartsTheSaveWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString makerHostname = qsl("Test-Headless-Host-Open-Map-Maker");
        QVERIFY2(HostManager::self()->addHost(makerHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        const QByteArray map = binaryMapOf(HostManager::self()->getHost(makerHostname), 9);
        HostManager::self()->deleteHost(makerHostname);

        const QString hostname = qsl("Test-Headless-Host-Open");
        const QString mapFolder = MudletApp::getMudletPath(enums::profileMapsPath, hostname);
        QVERIFY(QDir().mkpath(mapFolder));
        QFile mapFile(qsl("%1/headless.dat").arg(mapFolder));
        QVERIFY(mapFile.open(QIODevice::WriteOnly));
        mapFile.write(map);
        mapFile.close();

#ifdef INCLUDE_FONTS
        const QString fontPath = qsl("%1HeadlessMono.ttf").arg(MudletApp::getMudletPath(enums::profilePackagePath, hostname, qsl("HeadlessFontPack")));
        QVERIFY(QDir().mkpath(QFileInfo(fontPath).path()));
        QVERIFY(QFile::copy(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), fontPath));
#endif

        const auto script = [](const QString& name, const QString& code) {
            return qsl(R"(<Script isActive="yes" isFolder="no"><name>%1</name><packageName></packageName><script>%2</script><eventHandlerList/></Script>)").arg(name, code);
        };
        // A Host element in a module would be read into the profile's settings too
        const auto package = [](const QString& hostPackage, const QString& scripts) {
            return qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
%1<ScriptPackage>%2</ScriptPackage>
</MudletPackage>
)")
                    .arg(hostPackage, scripts);
        };
        QTemporaryDir moduleDir;
        QVERIFY(moduleDir.isValid());
        const QString modulePath = moduleDir.filePath(qsl("HeadlessModule.xml"));
        QFile moduleFile(modulePath);
        QVERIFY(moduleFile.open(QIODevice::WriteOnly));
        // The broken one would be reported on the console were the profile not marked as loading
        moduleFile.write(package(QString(), script(qsl("headlessModule"), qsl("headlessOpenModule = 'ran'")) + script(qsl("headlessBroken"), qsl("this is not lua"))).toUtf8());
        moduleFile.close();

        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, hostname);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        const QString hostDetails = qsl("<HostPackage><Host><mInstalledPackages><string>HeadlessFontPack</string></mInstalledPackages>"
                                        "<mInstalledModules><key>HeadlessModule</key><filepath>%1</filepath><globalSave>0</globalSave><priority>-1</priority></mInstalledModules></Host></HostPackage>")
                                            .arg(modulePath);
        // table.size() comes from LuaGlobal, and a priority -1 module is in before the profile's scripts start
        const QString profileScript = qsl(R"lua(headlessOpenModuleSeen = headlessOpenModule
registerAnonymousEventHandler("sysLoadEvent", function(_, fresh)
  headlessOpenLoad = tostring(fresh) .. " rooms:" .. table.size(getRooms()) .. " room9:" .. tostring(roomExists(9))
end))lua");
        saveFile.write(package(hostDetails, script(qsl("headlessOpen"), profileScript)).toUtf8());
        saveFile.close();
        QFile autolog(MudletApp::getMudletPath(enums::profileDataItemPath, hostname, qsl("autolog")));
        QVERIFY(autolog.open(QIODevice::WriteOnly));
        autolog.close();
        // An IRE game's address, which only a new profile gets the IRE driver fix for
        QFile url(MudletApp::getMudletPath(enums::profileDataItemPath, hostname, qsl("url")));
        QVERIFY(url.open(QIODevice::WriteOnly));
        QDataStream urlStream(&url);
        urlStream.setVersion(QDataStream::Qt_5_12);
        urlStream << qsl("achaea.com");
        url.close();

        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The saved profile did not open.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->mLoadedOk, qPrintable(host->mProfileLoadError));
        QCOMPARE(luaGlobalString(host, "headlessOpenModuleSeen"), qsl("ran"));
        QCOMPARE(luaGlobalString(host, "headlessOpenLoad"), qsl("true rooms:1 room9:true"));
        QVERIFY2(!host->mIsProfileLoadingSequence, "The profile is still marked as loading.");
        QVERIFY2(mainBufferHolds(host, qsl("loaded in offline mode")), "The offline load was not reported on the main console.");
        QVERIFY2(!mainBufferHolds(host, qsl("not working")), "Reinstalling the module as the profile opened reported its broken script.");
#ifdef INCLUDE_FONTS
        QVERIFY2(HostManager::self()->fonts().fontAlreadyLoaded(fontPath, hostname), "The installed package's font was not loaded.");
#endif
        QVERIFY2(host->mainConsoleModel().mLogToLogFile, "The profile's saved logging did not resume.");
        QVERIFY2(!host->mUSE_IRE_DRIVER_BUGFIX, "A profile with a save of its own was given the new profile settings.");

        QVERIFY2(!HostManager::self()->openProfile(hostname, false), "An open profile was opened again.");

        // Named for a listed game, so a profile with no save yet takes that game's address
        Host* fresh = HostManager::self()->openProfile(qsl("Achaea"), false);
        QVERIFY2(fresh, "A profile with no save did not open.");
        QVERIFY(fresh->mLoadedOk);
        QCOMPARE(fresh->getUrl(), qsl("achaea.com"));
        QVERIFY2(fresh->mUSE_IRE_DRIVER_BUGFIX, "A new profile for an IRE game did not get the IRE driver fix.");
        QVERIFY2(!fresh->mainConsoleModel().mLogToLogFile, "A profile that never logged started logging.");
        QCoreApplication::processEvents();
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Opening the profile created a widget.");
    }

    // The GUI's answers to the same Lua: the profile is loaded and started before loadProfile() returns,
    // and closeProfile() raises sysExitEvent and saves before it returns. The profile leaves the pool
    // later, from the event loop.
    void test_luaLoadsAndClosesAProfileWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString target = qsl("Test-Headless-Host-Lua-Opened");
        const QString unsaved = qsl("Test-Headless-Host-Lua-Unsaved");
        QVERIFY(writeRecordingProfile(target, true, recordPath));
        QVERIFY(writeRecordingProfile(unsaved, false, recordPath));

        // Where loadProfile() connects the profile to when it is not asked for offline
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        bool connected = false;
        connect(&server, &QTcpServer::newConnection, this, [&connected]() {
            connected = true;
        });
        QVERIFY(MudletApp::writeProfileData(target, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(target, qsl("port"), QString::number(server.serverPort())).first);

        const QString runnerName = qsl("Test-Headless-Host-Lua-Runner");
        QVERIFY2(HostManager::self()->addHost(runnerName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* runner = HostManager::self()->getHost(runnerName);
        const auto answer = [runner](const QString& call) {
            const QString chunk = qsl(R"lua(local r = {%1}
headlessAnswer = table.maxn(r) .. " " .. tostring(r[1]) .. " " .. tostring(r[2]))lua")
                                          .arg(call);
            return runner->getLuaInterpreter()->compileAndExecuteScript(chunk) ? luaGlobalString(runner, "headlessAnswer") : qsl("did not run");
        };

        QCOMPARE(answer(qsl("loadProfile('%1')").arg(target)), qsl("1 true nil"));
        Host* opened = HostManager::self()->getHost(target);
        QVERIFY2(opened, "loadProfile() left the profile out of the pool.");
        QVERIFY(!opened->hasConsoleView());
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysLoadEvent\n").arg(target));
        QTRY_VERIFY_WITH_TIMEOUT(connected, 10000);
        QCOMPARE(answer(qsl("loadProfile('%1', true)").arg(target)), qsl("2 nil loadProfile: profile '%1' is already loaded").arg(target));

        const QPointer<Host> pOpened(opened);
        opened->mpMap->addRoom(5);
        QVERIFY(clearMapSaves(target));
        QCOMPARE(answer(qsl("closeProfile('%1'), getProfiles()['%1'].loaded").arg(target)), qsl("2 true true"));
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysLoadEvent\n%1 sysExitEvent\n").arg(target));
        QVERIFY2(pOpened, "The profile went before the event loop ran.");
        QVERIFY2(!pOpened->currentlySavingProfile(), "closeProfile() returned with the profile's save still running.");
        QVERIFY2(newestSaveIsComplete(target), "closeProfile() returned before the profile's save was whole on disk.");
        QCOMPARE(saveCount(target), 2);
        QVERIFY2(newestMapSaveHoldsRoom(target, 5), "closeProfile() did not save the profile's map.");
        QTRY_VERIFY_WITH_TIMEOUT(!pOpened, 5000);
        QVERIFY(!HostManager::self()->hostLoaded(target));
        QCOMPARE(answer(qsl("closeProfile('%1')").arg(target)), qsl("2 nil closeProfile: profile '%1' is not loaded").arg(target));

        // The GUI asks whether to save a profile that does not save on exit. Nobody can answer that here,
        // so it closes as for the answer No.
        QCOMPARE(answer(qsl("loadProfile('%1', true)").arg(unsaved)), qsl("1 true nil"));
        const QPointer<Host> pUnsaved(HostManager::self()->getHost(unsaved));
        // As the GUI's: a close asked for while the profile is already closing is ignored
        QVERIFY(pUnsaved->getLuaInterpreter()->compileAndExecuteScript(
                qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() headlessRecord("closeProfile in exit " .. tostring(closeProfile())) end))lua")));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(qsl("HostManager: asked to close profile \"%1\" while it is running a nested event loop, ignoring").arg(unsaved)));
        QCOMPARE(answer(qsl("closeProfile('%1')").arg(unsaved)), qsl("1 true nil"));
        QTRY_VERIFY_WITH_TIMEOUT(!pUnsaved, 5000);
        QCOMPARE(saveCount(unsaved), 1);
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysLoadEvent\n%1 sysExitEvent\n%2 started\n%2 sysLoadEvent\n%2 sysExitEvent\n%2 closeProfile in exit true\n").arg(target, unsaved));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Loading or closing a profile created a widget.");
        HostManager::self()->deleteHost(runnerName);
    }

    // As the GUI's tab close: sysExitEvent under the script that asked, and the profile goes only once
    // that script has finished, even if it runs the event loop after asking
    void test_luaClosesItsOwnProfileWithNoMainWindow()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString hostname = qsl("Test-Headless-Host-Lua-Self-Closed");
        const QString other = qsl("Test-Headless-Host-Lua-Closed-After");
        QVERIFY(writeRecordingProfile(hostname, true, recordPath));
        QVERIFY(writeRecordingProfile(other, true, recordPath));
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The saved profile did not open.");
        QVERIFY2(HostManager::self()->openProfile(other, false), "The saved profile did not open.");
        QVERIFY(QFile::remove(recordPath));
        const QPointer<Host> pHost(host);

        // The other profile's save waits on the event loop, where a close that did not wait for this
        // script would delete its profile under it
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local ok, err = closeProfile()
headlessRecord("closeProfile " .. tostring(ok) .. " " .. tostring(err))
headlessRecord("closeProfile other " .. tostring(closeProfile([[%1]])))
headlessRecord("finished")
)lua")
                                                                                    .arg(other));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(readRecord(recordPath), qsl("%1 sysExitEvent\n%1 closeProfile true nil\n%2 sysExitEvent\n%1 closeProfile other true\n%1 finished\n").arg(hostname, other));
        QCOMPARE(saveCount(hostname), 2);
        QCOMPARE(saveCount(other), 2);
        QVERIFY2(pHost, "The profile went before the event loop ran.");
        QTRY_VERIFY_WITH_TIMEOUT(!pHost, 5000);
        QVERIFY(!HostManager::self()->hostLoaded(hostname));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self()->hostLoaded(other), 5000);

        // A profile already closing is not closed, or saved, again
        const QString again = qsl("Test-Headless-Host-Lua-Closed-Twice");
        Host* closedTwice = HostManager::self()->openProfile(again, false);
        QVERIFY2(closedTwice, "The profile did not open.");
        QVERIFY(HostManager::self()->requestProfileClose(again));
        QVERIFY2(!closedTwice->closeWithNoView(), "A profile already closing was closed again.");
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self()->hostLoaded(again), 5000);
    }

    // As in the GUI: the load runs on, but the closed profile's handlers hear no sysLoadEvent
    void test_luaClosesAProfileFromItsOwnLoadWithNoMainWindow()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString hostname = qsl("Test-Headless-Host-Lua-Closed-At-Load");
        QVERIFY(writeRecordingProfile(hostname, true, recordPath));
        QFile save(qsl("%1/2020-01-01#00-00-00.xml").arg(MudletApp::getMudletPath(enums::profileXmlFilesPath, hostname)));
        QVERIFY(save.open(QIODevice::ReadOnly));
        QByteArray xml = save.readAll();
        save.close();
        xml.replace("</script>", "\nheadlessRecord(&quot;closeProfile &quot; .. tostring(closeProfile()))</script>");
        QVERIFY(save.open(QIODevice::WriteOnly));
        save.write(xml);
        save.close();

        const QPointer<Host> pHost(HostManager::self()->openProfile(hostname, false));
        QVERIFY2(pHost, "The saved profile did not open.");
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysExitEvent\n%1 closeProfile true\n").arg(hostname));
        QTRY_VERIFY_WITH_TIMEOUT(!pHost, 5000);
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysExitEvent\n%1 closeProfile true\n").arg(hostname));
    }

    // A close that closeMudlet() posts can wait for a save whose writes ended, and posted that they had, before
    // the close ran; an event loop pass run from a posted call need not deliver those, and the wait must end
    void test_aSaveIsWaitedOutFromAPostedCall()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString hostname = qsl("Test-Headless-Host-Save-Waited-From-A-Posted-Call");
        QVERIFY(writeRecordingProfile(hostname, true, recordDir.filePath(qsl("record.txt"))));
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The saved profile did not open.");
        host->waitForProfileSave();

        // Every pool thread held, so the save's writes end, and post that they have, only after the call below
        auto* pool = QThreadPool::globalInstance();
        const int threads = pool->maxThreadCount();
        QSemaphore held;
        QSemaphore release;
        for (int i = 0; i < threads; ++i) {
            pool->start([&held, &release]() {
                held.release();
                release.acquire();
            });
        }
        QVERIFY(held.tryAcquire(threads, 10000));
        const bool started = std::get<0>(host->saveProfile());
        bool waited = false;
        bool saveEnded = false;
        qint64 waitMs = -1;
        QMetaObject::invokeMethod(
                host,
                [host, &waited, &saveEnded, &waitMs]() {
                    QElapsedTimer clock;
                    clock.start();
                    host->waitForProfileSave();
                    saveEnded = !host->currentlySavingProfile();
                    waitMs = clock.elapsed();
                    waited = true;
                },
                Qt::QueuedConnection);
        release.release(threads);
        QVERIFY(started);
        QVERIFY(pool->waitForDone(10000));

        QTRY_VERIFY_WITH_TIMEOUT(waited, 60000);
        QVERIFY2(saveEnded, qPrintable(qsl("The wait gave up on a save that had ended, after %1 ms.").arg(waitMs)));
        QVERIFY2(waitMs < 10000, qPrintable(qsl("The wait took %1 ms.").arg(waitMs)));
        HostManager::self()->deleteHost(hostname);
    }

    // A close save turned away because the wait gave up on an earlier save, which may predate the session's last
    // changes, fails the close rather than counting that earlier save as written
    void test_aCloseSaveTurnedAwayFailsTheClose()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString hostname = qsl("Test-Headless-Host-Close-Save-Turned-Away");
        QVERIFY(writeRecordingProfile(hostname, true, recordDir.filePath(qsl("record.txt"))));
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The saved profile did not open.");
        host->waitForProfileSave();
        const qsizetype savesBefore = saveCount(hostname);
        const QPointer<Host> pHost(host);

        // A save that has written everything but never reports itself finished, given up on quickly
        const QString stuck = qsl("never reports");
        host->writers.insert(stuck, std::make_shared<XMLexport>(host));
        host->mProfileSaveNotificationTimeout = std::chrono::milliseconds(50);
        const auto unstick = qScopeGuard([pHost, stuck]() {
            if (pHost) {
                pHost->writers.remove(stuck);
            }
        });
        bool closeSaveFailed = false;
        connect(host, &Host::closeSaveFailed, host, [host, stuck, &closeSaveFailed]() {
            closeSaveFailed = true;
            host->writers.remove(stuck);
        });

        QVERIFY(HostManager::self()->requestProfileClose(hostname));
        QVERIFY2(closeSaveFailed, "A close whose own save was turned away was not reported as failed.");
        QCOMPARE(saveCount(hostname), savesBefore);
        QTRY_VERIFY_WITH_TIMEOUT(!pHost, 5000);
    }

    void test_deleteHostKeepsAProfileReopenedUnderItsExitHandler()
    {
        const QString hostname = qsl("Test-Headless-Host-Exit-Reopened");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        // Stands in for an embedder's slot that runs while an exit handler pumps events
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "reopenProfile", [](lua_State* L) -> int {
            const QString name = QString::fromUtf8(lua_tostring(L, 1));
            HostManager::self()->deleteHost(name);
            lua_pushboolean(L, HostManager::self()->addHost(name, QString(), QString(), QString()));
            return 1;
        });
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onReopenExit"));
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onReopenExit() assert(reopenProfile(getProfileName())) end")), "The Lua chunk did not run.");

        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->getHost(hostname), "Closing the profile also removed the one reopened under its name.");
        HostManager::self()->deleteHost(hostname);
    }

    // A profile opened then would be deleted with the pool without its sysExitEvent
    void test_aPoolBeingDestroyedOpensNoProfile()
    {
        static HostManager* spPool = nullptr;
        static int sOpened = -1;
        auto pool = std::make_unique<HostManager>();
        spPool = pool.get();
        const QString hostname = qsl("Test-Headless-Host-Pool-Going");
        QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = pool->getHost(hostname);
        // Stands in for an embedder's slot that runs while an exit handler pumps events
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "openInPool", [](lua_State* L) -> int {
            sOpened = spPool->openProfile(QString::fromUtf8(lua_tostring(L, 1)), false) ? 1 : 0;
            return 0;
        });
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onPoolGoing"));
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onPoolGoing() openInPool('Test-Headless-Host-Pool-Late') end")), "The Lua chunk did not run.");
        pool.reset();
        QCOMPARE(sOpened, 0);
    }

    // An embedder may close every profile from a slot that runs while one of them is closing, here
    // under the script of another profile that asked for that close
    void test_closeAllProfilesUnderAClosingProfilesExitHandler()
    {
        static HostManager* spPool = nullptr;
        static Host* spClosing = nullptr;
        static int sClosedAgain = -1;
        auto pool = std::make_unique<HostManager>();
        spPool = pool.get();
        const QString closingName = qsl("Test-Headless-Host-Close-All-Under-Exit");
        const QString askingName = qsl("Test-Headless-Host-Close-All-Asking");
        QVERIFY2(pool->addHost(closingName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(pool->addHost(askingName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        spClosing = pool->getHost(closingName);
        Host* asking = pool->getHost(askingName);
        // closeProfile() would reach the first pool
        lua_register(asking->getLuaInterpreter()->getLuaGlobalState(), "closeInPool", [](lua_State* L) -> int {
            lua_pushboolean(L, spPool->requestProfileClose(QString::fromUtf8(lua_tostring(L, 1))));
            return 1;
        });
        lua_register(spClosing->getLuaInterpreter()->getLuaGlobalState(), "closeAllInPool", [](lua_State*) -> int {
            sClosedAgain = spClosing->closeWithNoView() ? 1 : 0;
            spPool->closeAllProfiles();
            return 0;
        });
        spClosing->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onCloseAllUnderExit"));
        QVERIFY2(spClosing->getLuaInterpreter()->compileAndExecuteScript(qsl("function onCloseAllUnderExit() closeAllInPool() end")), "The Lua chunk did not run.");
        const QPointer<Host> pClosing(spClosing);
        const QPointer<Host> pAsking(asking);

        QVERIFY(asking->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessClosed = tostring(closeInPool([[%1]]))").arg(closingName)));
        QVERIFY2(pAsking, "The profile was deleted under its own script.");
        QCOMPARE(luaGlobalString(asking, "headlessClosed"), qsl("true"));
        QCOMPARE(sClosedAgain, 0);
        QTRY_VERIFY_WITH_TIMEOUT(!pClosing && !pAsking, 5000);
        QCOMPARE(pool->getHostCount(), 0);
    }

    // An embedder may close every profile from a slot that a map operation's event loop runs
    void test_closeAllProfilesUnderAMapOperation()
    {
        static HostManager* spPool = nullptr;
        auto pool = std::make_unique<HostManager>();
        spPool = pool.get();
        const QString hostname = qsl("Test-Headless-Host-Close-All-Under-Export");
        QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = pool->getHost(hostname);
        connect(
                host->mpMap.data(),
                &TMap::signal_mapProgressSetValue,
                this,
                []() {
                    spPool->closeAllProfiles();
                },
                Qt::SingleShotConnection);
        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());
        const QPointer<Host> pHost(host);
        const auto [exported, exportMessage] = host->mpMap->writeJsonMapFile(exportDir.filePath(qsl("map.json")));
        QVERIFY2(pHost, "The profile was deleted under its map operation.");
        QVERIFY(!exported);
        QCOMPARE(exportMessage, qsl("aborted by user"));
        QTRY_VERIFY_WITH_TIMEOUT(!pHost, 5000);
    }

    // Nor one deleted meanwhile while its own script, which asked for the close, still runs
    void test_closeAllProfilesKeepsADeletedProfileUntilItsScriptEnds()
    {
        static HostManager* spPool = nullptr;
        auto pool = std::make_unique<HostManager>();
        spPool = pool.get();
        const QString askingName = qsl("Test-Headless-Host-Close-All-Asks");
        const QString deletingName = qsl("Test-Headless-Host-Close-All-Deletes");
        QVERIFY2(pool->addHost(askingName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(pool->addHost(deletingName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* asking = pool->getHost(askingName);
        Host* deleting = pool->getHost(deletingName);
        // Stand in for an embedder's slots
        lua_register(asking->getLuaInterpreter()->getLuaGlobalState(), "closeAllInPool", [](lua_State*) -> int {
            spPool->closeAllProfiles();
            return 0;
        });
        // Where the release is first tried
        registerRunEventLoop(asking);
        lua_register(deleting->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            spPool->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        deleting->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onDeleteAsking"));
        QVERIFY2(deleting->getLuaInterpreter()->compileAndExecuteScript(qsl("function onDeleteAsking() deleteFromPool([[%1]]) end").arg(askingName)), "The Lua chunk did not run.");
        const QPointer<Host> pAsking(asking);
        QVERIFY(asking->getLuaInterpreter()->compileAndExecuteScript(qsl("closeAllInPool() runEventLoop() headlessAfterCloseAll = string.rep('x', 1000)")));
        QVERIFY2(pAsking, "The profile was deleted under its own script.");
        QTRY_VERIFY_WITH_TIMEOUT(!pAsking, 5000);
        QCOMPARE(pool->getHostCount(), 0);
    }

    // Nor while it runs a nested event loop, as a save question would
    void test_closeAllProfilesLeavesAProfileInANestedEventLoopUntilItEnds()
    {
        auto pool = std::make_unique<HostManager>();
        const QString hostname = qsl("Test-Headless-Host-Close-All-Pumping");
        QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = pool->getHost(hostname);
        const QPointer<Host> pHost(host);
        host->getLuaInterpreter()->enterNestedEventLoop();
        pool->closeAllProfiles();
        QTest::qWait(100);
        QVERIFY2(pHost, "The profile was deleted under its nested event loop.");
        host->getLuaInterpreter()->leaveNestedEventLoop();
        QTRY_VERIFY_WITH_TIMEOUT(!pHost, 5000);
    }

    // A profile an embedder deletes while closing every profile is not then closed, or saved, as well
    void test_closeAllProfilesLeavesAProfileDeletedMeanwhile()
    {
        static HostManager* spPool = nullptr;
        auto pool = std::make_unique<HostManager>();
        spPool = pool.get();
        const QString closingName = qsl("Test-Headless-Host-Close-All-Deleting");
        // Named to come after the closing one, so the pass reaches it after the handler deleted it
        const QString deletedName = qsl("Test-Headless-Host-Close-All-Removed");
        Host* closing = pool->openProfile(closingName, false);
        QVERIFY2(closing, "The profile did not open.");
        QVERIFY2(pool->openProfile(deletedName, false), "The profile did not open.");
        // Stands in for an embedder's slot that runs while an exit handler pumps events
        lua_register(closing->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            spPool->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        closing->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onDeleteOther"));
        QVERIFY2(closing->getLuaInterpreter()->compileAndExecuteScript(qsl("function onDeleteOther() deleteFromPool([[%1]]) end").arg(deletedName)), "The Lua chunk did not run.");
        const QPointer<Host> pClosing(closing);
        pool->closeAllProfiles();
        QCOMPARE(pool->getHostCount(), 0);
        QVERIFY2(!pClosing, "A profile with no script running outlived its close.");
        QCOMPARE(saveCount(closingName), 1);
        QCOMPARE(saveCount(deletedName), 0);
    }

    // An embedder holding a profile past its pool's end
    void test_aProfileClosedThenHeldPastItsPool()
    {
        auto pool = std::make_unique<HostManager>();
        const QString hostname = qsl("Test-Headless-Host-Held-Past-Pool");
        QVERIFY2(pool->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QList<QSharedPointer<Host>> held = pool->hostList();
        const QPointer<Host> pHost(held.first().data());
        QVERIFY(pool->requestProfileClose(hostname));
        pool.reset();
        QVERIFY2(pHost, "The held profile went with its pool.");
        QTest::qWait(200);
        held.clear();
        QVERIFY(!pHost);
    }

    // As deleteHost(): a profile reopened under the closing one's name is not the one that goes
    void test_requestProfileCloseKeepsAProfileReopenedUnderItsExitHandler()
    {
        const QString hostname = qsl("Test-Headless-Host-Close-Reopened");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        // Stands in for an embedder's slot that runs while an exit handler pumps events
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "reopenProfile", [](lua_State* L) -> int {
            const QString name = QString::fromUtf8(lua_tostring(L, 1));
            HostManager::self()->deleteHost(name);
            lua_pushboolean(L, HostManager::self()->addHost(name, QString(), QString(), QString()));
            return 1;
        });
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "closeSelf", [](lua_State* L) -> int {
            lua_pushboolean(L, HostManager::self()->requestProfileClose(QString::fromUtf8(lua_tostring(L, 1))));
            return 1;
        });
        // Where the release is first tried
        registerRunEventLoop(host);
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onReopenExit"));
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onReopenExit() assert(reopenProfile(getProfileName())) end")), "The Lua chunk did not run.");

        // From the profile's own script, which the profile taken out of the pool has to outlast
        const QPointer<Host> pClosing(host);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(closeSelf(getProfileName())) runEventLoop() headlessAfterClose = string.rep('x', 1000)")),
                 "The Lua chunk did not run.");
        QVERIFY2(pClosing, "The profile was deleted under its own script.");
        const QPointer<Host> pReopened(HostManager::self()->getHost(hostname));
        QVERIFY2(pReopened && pReopened != host, "The profile was not reopened under its name.");
        QTRY_VERIFY_WITH_TIMEOUT(!pClosing, 5000);
        QVERIFY2(pReopened, "Closing the profile also removed the one reopened under its name.");
        HostManager::self()->deleteHost(hostname);
    }

    // As in the GUI, whose save question runs the event loop first: a reset the close comes after is moot
    void test_requestProfileCloseDropsAResetOfTheProfile()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString name = qsl("Test-Headless-Host-Reset-Then-Close");
        QVERIFY(writeRecordingProfile(name, false, recordPath));
        Host* host = HostManager::self()->openProfile(name, false);
        QVERIFY2(host, "The saved profile did not open.");
        const QPointer<Host> pHost(host);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessRecord('reset ' .. tostring(resetProfile())) headlessRecord('close ' .. tostring(closeProfile(getProfileName())))")),
                 "The Lua chunk did not run.");
        QTRY_VERIFY_WITH_TIMEOUT(!pHost, 5000);
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysLoadEvent\n%1 reset true\n%1 sysExitEvent\n%1 close true\n").arg(name));
    }

    // Near the end, as it closes every profile in the pool. As the GUI's: once the calling script has finished,
    // each profile gets sysExitEvent and is saved, whatever its save on exit says, and the application quits.
    void test_closeMudletClosesEveryProfileAndQuitsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString first = qsl("Test-Headless-Host-Quit-1");
        const QString second = qsl("Test-Headless-Host-Quit-2");
        const QString closedFirst = qsl("Test-Headless-Host-Quit-3");
        QVERIFY(writeRecordingProfile(first, true, recordPath));
        QVERIFY(writeRecordingProfile(second, false, recordPath));
        QVERIFY(writeRecordingProfile(closedFirst, true, recordPath));
        // The first profile's load asks to quit, then reads a JSON map, which runs the event loop
        QFile save(qsl("%1/2020-01-01#00-00-00.xml").arg(MudletApp::getMudletPath(enums::profileXmlFilesPath, first)));
        QVERIFY(save.open(QIODevice::ReadOnly));
        QByteArray xml = save.readAll();
        save.close();
        xml.replace("</script>", "\ncloseMudlet() headlessRecord(&quot;closeMudlet returned&quot;)</script>");
        QVERIFY(save.open(QIODevice::WriteOnly));
        save.write(xml);
        save.close();
        const QString makerName = qsl("Test-Headless-Host-Quit-Map-Maker");
        QVERIFY2(HostManager::self()->addHost(makerName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        TMap* makerMap = HostManager::self()->getHost(makerName)->mpMap.data();
        makerMap->addRoom(5);
        makerMap->setRoomArea(5, makerMap->mpRoomDB->addArea(qsl("Headless Quit Area")));
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileMapsPath, first)));
        QVERIFY(makerMap->writeJsonMapFile(qsl("%1/headless.json").arg(MudletApp::getMudletPath(enums::profileMapsPath, first))).first);
        HostManager::self()->deleteHost(makerName);

        QVERIFY2(HostManager::self()->openProfile(second, false), "The saved profile did not open.");
        QVERIFY2(HostManager::self()->openProfile(closedFirst, false), "The saved profile did not open.");
        QVERIFY(QFile::remove(recordPath));
        // As in the GUI, the close waits for the load, rather than cutting its map short
        Host* host = HostManager::self()->openProfile(first, false);
        QVERIFY2(host, "The saved profile did not open.");
        QVERIFY2(host->mpMap->mpRoomDB->getRoom(5), "The profile's map was cut short as it loaded.");
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 closeMudlet returned\n%1 sysLoadEvent\n").arg(first));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  headlessRecord("closeProfile in exit " .. tostring(closeProfile([[%1]])))
end))lua")
                                                                           .arg(second)));
        const int openBefore = HostManager::self()->getHostCount();
        QVERIFY(clearMapSaves(first));
        // Nothing waits for a save once its profile has gone
        QStringList saveOrder;
        // Goes before saveOrder does, so a test that ends early leaves no connection to it
        QObject saveWatch;
        for (const QString& name : {first, second, closedFirst}) {
            Host* pClosing = HostManager::self()->getHost(name);
            connect(pClosing, &Host::profileSaveFinished, &saveWatch, [&saveOrder, name]() {
                saveOrder << qsl("%1 saved").arg(name);
            });
            connect(pClosing, &QObject::destroyed, &saveWatch, [&saveOrder, name]() {
                saveOrder << qsl("%1 gone").arg(name);
            });
        }

        // As in the GUI, nor while a profile runs a nested event loop, as its save question would
        Host* pumping = HostManager::self()->getHost(second);
        pumping->getLuaInterpreter()->enterNestedEventLoop();
        QTest::qWait(200);
        pumping->getLuaInterpreter()->leaveNestedEventLoop();
        QCOMPARE(HostManager::self()->getHostCount(), openBefore);

        // Where the close then runs, as the GUI's does
        registerRunEventLoop(host);
        const QPointer<Host> pHost(host);
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
runEventLoop()
headlessRecord("closeProfile " .. tostring(closeProfile([[%1]])))
)lua")
                                                                                    .arg(closedFirst));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QVERIFY2(pHost, "The profile was deleted under its script.");
        QCOMPARE(HostManager::self()->getHostCount(), 1);

        // The close can run from an event loop pump before the embedder's exec(), and the quit then
        // waits for that exec(), the way an embedder learns of it
        QElapsedTimer pumped;
        pumped.start();
        while (HostManager::self()->getHostCount() && pumped.elapsed() < 10000) {
            QCoreApplication::processEvents();
        }
        QCOMPARE(HostManager::self()->getHostCount(), 0);
        QCOMPARE(execUntilQuit(std::chrono::seconds(20)), 0);
        // Once the application has quit, it is not asked to again
        QCOMPARE(execUntilQuit(std::chrono::milliseconds(300)), 2);
        QCOMPARE(readRecord(recordPath),
                 qsl("%1 started\n%1 closeMudlet returned\n%1 sysLoadEvent\n%1 sysExitEvent\n%2 sysExitEvent\n%1 closeProfile in exit true\n%3 sysExitEvent\n%1 closeProfile nil\n")
                         .arg(first, second, closedFirst));
        QCOMPARE(saveCount(first), 2);
        QCOMPARE(saveCount(second), 2);
        QCOMPARE(saveCount(closedFirst), 2);
        for (const QString& name : {first, second, closedFirst}) {
            QCOMPARE(saveOrder.count(qsl("%1 saved").arg(name)), 1);
            QVERIFY2(saveOrder.indexOf(qsl("%1 saved").arg(name)) < saveOrder.indexOf(qsl("%1 gone").arg(name)), qPrintable(saveOrder.join(qsl(", "))));
        }
        QVERIFY2(newestMapSaveHoldsRoom(first, 5), "closeMudlet() did not save the profile's map.");
    }
    // After the one above, as it quits too. A profile an exit handler takes out of the pool as its own
    // script runs is deleted before the quit, as nothing would delete it after, and a profile that script
    // opens meanwhile is closed and saved too.
    void test_closeMudletQuitsOnceAProfileTakenOutOfThePoolIsDeleted()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString heldName = qsl("Test-Headless-Host-Quit-Held");
        const QString takingName = qsl("Test-Headless-Host-Quit-Takes");
        const QString openedName = qsl("Test-Headless-Host-Quit-Opened-Meanwhile");
        QVERIFY(writeRecordingProfile(openedName, false, recordPath));
        QVERIFY2(HostManager::self()->addHost(heldName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(HostManager::self()->addHost(takingName, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* held = HostManager::self()->getHost(heldName);
        Host* taking = HostManager::self()->getHost(takingName);
        // Stands in for an embedder's slot
        lua_register(taking->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            HostManager::self()->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        taking->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onExitTakeHeld"));
        QVERIFY2(taking->getLuaInterpreter()->compileAndExecuteScript(qsl("function onExitTakeHeld() deleteFromPool([[%1]]) end").arg(heldName)), "The Lua chunk did not run.");
        // Where the close and the quit then run
        registerRunEventLoop(held);
        const QPointer<Host> pHeld(held);
        bool heldRan = false;
        // Goes before heldRan does, so a test that ends early leaves no timer writing to it
        QObject runWatch;
        QTimer::singleShot(0, &runWatch, [pHeld, &heldRan, openedName]() {
            heldRan = pHeld && pHeld->getLuaInterpreter()->compileAndExecuteScript(qsl("runEventLoop() assert(loadProfile([[%1]], true))").arg(openedName));
        });
        QVERIFY2(taking->getLuaInterpreter()->compileAndExecuteScript(qsl("closeMudlet()")), "The Lua chunk did not run.");

        QCOMPARE(execUntilQuit(std::chrono::seconds(20)), 0);
        QVERIFY2(heldRan, "The Lua chunk did not run.");
        QVERIFY2(!pHeld, "The application quit with a profile taken out of the pool still undeleted.");
        QCOMPARE(HostManager::self()->getHostCount(), 0);
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysLoadEvent\n%1 sysExitEvent\n").arg(openedName));
        QCOMPARE(saveCount(openedName), 2);
    }

    // After the ones above, as it quits too. As in the GUI, whose alert runs the event loop before its
    // close: a reset asked for first ends before the close
    void test_closeMudletLetsAResetAskedForFirstEnd()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString name = qsl("Test-Headless-Host-Quit-Reset");
        QVERIFY(writeRecordingProfile(name, false, recordPath));
        Host* host = HostManager::self()->openProfile(name, false);
        QVERIFY2(host, "The saved profile did not open.");
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("closeMudlet() headlessRecord('reset ' .. tostring(resetProfile()))")), "The Lua chunk did not run.");

        QCOMPARE(execUntilQuit(std::chrono::seconds(20)), 0);
        QCOMPARE(HostManager::self()->getHostCount(), 0);
        QCOMPARE(readRecord(recordPath), qsl("%1 started\n%1 sysLoadEvent\n%1 reset true\n%1 started\n%1 sysLoadEvent\n%1 sysExitEvent\n").arg(name));
    }

    // After the ones above, as it quits too. As in the GUI, the other profiles close while a closed profile
    // still runs its script, even one whose reset its close made moot
    void test_closeMudletDoesNotWaitForTheResetOfAClosedProfile()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString closedName = qsl("Test-Headless-Host-Quit-Closed-Reset");
        const QString otherName = qsl("Test-Headless-Host-Quit-Other");
        QVERIFY(writeRecordingProfile(closedName, false, recordPath));
        QVERIFY(writeRecordingProfile(otherName, false, recordPath));
        Host* closed = HostManager::self()->openProfile(closedName, false);
        QVERIFY2(closed, "The saved profile did not open.");
        Host* other = HostManager::self()->openProfile(otherName, false);
        QVERIFY2(other, "The saved profile did not open.");
        QVERIFY(QFile::remove(recordPath));
        registerRunEventLoop(closed);
        const QPointer<Host> pClosed(closed);
        bool closedRan = false;
        // Goes before closedRan does, so a test that ends early leaves no timer writing to it
        QObject runWatch;
        QTimer::singleShot(0, &runWatch, [pClosed, &closedRan]() {
            closedRan = pClosed
                        && pClosed->getLuaInterpreter()->compileAndExecuteScript(
                                qsl("closeProfile(getProfileName()) headlessRecord('reset ' .. tostring(resetProfile())) runEventLoop() headlessRecord('script ends')"));
        });
        QVERIFY2(other->getLuaInterpreter()->compileAndExecuteScript(qsl("closeMudlet()")), "The Lua chunk did not run.");

        QCOMPARE(execUntilQuit(std::chrono::seconds(20)), 0);
        QVERIFY2(closedRan, "The Lua chunk did not run.");
        QCOMPARE(readRecord(recordPath), qsl("%1 sysExitEvent\n%1 reset true\n%2 sysExitEvent\n%1 script ends\n").arg(closedName, otherName));
        QTRY_VERIFY_WITH_TIMEOUT(!pClosed, 5000);
        QCOMPARE(HostManager::self()->getHostCount(), 0);
    }

    // After the ones above, as it quits too. As in the GUI, whose alert runs the event loop only once: a reset
    // asked for by a script still running is not waited for, and the close makes it moot
    void test_closeMudletDoesNotWaitForAResetItsScriptHolds()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = recordDir.filePath(qsl("record.txt"));
        const QString resettingName = qsl("Test-Headless-Host-Quit-Resetting");
        const QString otherName = qsl("Test-Headless-Host-Quit-Beside-Reset");
        QVERIFY(writeRecordingProfile(resettingName, false, recordPath));
        QVERIFY(writeRecordingProfile(otherName, false, recordPath));
        Host* resetting = HostManager::self()->openProfile(resettingName, false);
        QVERIFY2(resetting, "The saved profile did not open.");
        QVERIFY2(HostManager::self()->openProfile(otherName, false), "The saved profile did not open.");
        QVERIFY(QFile::remove(recordPath));
        registerRunEventLoop(resetting);
        const QPointer<Host> pResetting(resetting);
        bool resettingRan = false;
        // Goes before resettingRan does, so a test that ends early leaves no timer writing to it
        QObject runWatch;
        QTimer::singleShot(0, &runWatch, [pResetting, &resettingRan]() {
            resettingRan =
                    pResetting
                    && pResetting->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessRecord('reset ' .. tostring(resetProfile())) closeMudlet() runEventLoop() headlessRecord('script ends')"));
        });

        QCOMPARE(execUntilQuit(std::chrono::seconds(20)), 0);
        QVERIFY2(resettingRan, "The Lua chunk did not run.");
        QTRY_VERIFY_WITH_TIMEOUT(!pResetting, 5000);
        QCOMPARE(HostManager::self()->getHostCount(), 0);
        QStringList record = readRecord(recordPath).split(QChar('\n'), Qt::SkipEmptyParts);
        QCOMPARE(record.takeFirst(), qsl("%1 reset true").arg(resettingName));
        QCOMPARE(record.takeLast(), qsl("%1 script ends").arg(resettingName));
        record.sort();
        QCOMPARE(record, (QStringList{qsl("%1 sysExitEvent").arg(otherName), qsl("%1 sysExitEvent").arg(resettingName)}));
    }

    // Last, after the closeMudlet() tests have closed every profile: its profile stays open, for cleanupTestCase() to
    // find its window in the layout the app's close saves
    void test_headlessWindowLayoutSavedAsAppCloses()
    {
        const QString& hostname = mAppCloseHostname;
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
openUserWindow("wlA", false, false, "f")
resizeWindow("wlA", 131, 61)
moveWindow("wlA", 9, 10)
function onWlAppExit() resizeWindow("wlA", 132, 62) end
registerAnonymousEventHandler("sysExitEvent", "onWlAppExit")
)lua")));
        mAppCloseLayoutChecked = true;
    }
};

#include "HeadlessHostSmokeTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessHostSmokeTest)
