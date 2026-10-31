/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

// HostManager::self() is dereferenced unguarded all over the tree, so what a
// second instance does to it matters: the application holds one as a value
// member, but the class is publicly constructible and a test exercising the
// host pool on its own is the obvious next thing to write.
//
// The last test covers the other end of that lifetime. The header promises the
// accessor answers "until its members are torn down", and mHostPool is the
// member that matters: every loaded profile's ~Host() runs as it goes, and
// several repointed call sites reach HostManager::self() from there.

#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "SpeechRecognizer.h"
#include "SpeechRecognizerFactory.h"
#include "TLuaInterpreter.h"
#include "TSpeechBridge.h"
#include "discord.h"
#include "mudlet.h"

#include <QDir>
#include <QFile>
#include <QMetaEnum>
#include <QPointer>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSettings>
#include <QTemporaryDir>

#include <memory>

// A stand-in engine, so a profile can be listening as the application goes
class AccessorStandInRecognizer : public SpeechRecognizer
{
public:
    explicit AccessorStandInRecognizer(QObject* parent = nullptr)
    : SpeechRecognizer(parent)
    {
    }

    bool initialize(const QString&) override
    {
        setState(State::Ready);
        return true;
    }
    QString currentLanguage() const override { return qsl("en"); }
    bool setLanguage(const QString&) override { return true; }
    QString backendName() const override { return qsl("AccessorStandIn"); }
    QString backendVersion() const override { return qsl("1.0"); }
    bool setSensitivity(Sensitivity) override { return true; }
    Sensitivity sensitivity() const override { return Sensitivity::Default; }

protected:
    void doStartListening() override { setState(State::Listening); }
    void doStopListening() override { setState(State::Ready); }
    void doCancel() override { setState(State::Ready); }
};

class HostManagerAccessorTest : public QObject
{
    Q_OBJECT

    // Every pool made here settles the config root, which then stays for the process
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    // What the listening profile's scripts and the engine are told from the start on
    inline static QStringList smScriptStates;
    inline static QStringList smEngineStates;
    // Whether a settings store existed as the refused portable.txt was reported
    inline static bool smReported = false;
    inline static bool smStoreExistedWhenReported = false;
    inline static QtMessageHandler smPreviousHandler = nullptr;

    // Model loaded, as with no engine library on macOS each stt.start() builds another
    static auto installStandInFactory()
    {
        SpeechRecognizerFactory::setFactoryOverride([](QObject* parent) -> SpeechRecognizer* {
            auto* engine = new AccessorStandInRecognizer(parent);
            engine->initialize(QString());
            return engine;
        });
        return qScopeGuard([]() {
            SpeechRecognizerFactory::setFactoryOverride(nullptr);
        });
    }

    static bool startListening(Host* pHost)
    {
        smScriptStates.clear();
        smEngineStates.clear();
        if (!pHost) {
            return false;
        }
        lua_register(pHost->getLuaInterpreter()->getLuaGlobalState(), "recordSttState", [](lua_State* L) -> int {
            smScriptStates << QString::fromUtf8(lua_tostring(L, 2));
            return 0;
        });
        pHost->registerAnonymousEventHandler(qsl("sysSTTStateChanged"), qsl("recordSttState"));
        if (!pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(stt.start())")) || TSpeechBridge::instance()->microphoneOwner() != pHost) {
            return false;
        }
        connect(TSpeechBridge::instance()->speechRecognizer(), &SpeechRecognizer::stateChanged, qApp, [](SpeechRecognizer::State state) {
            smEngineStates << QString::fromLatin1(QMetaEnum::fromType<SpeechRecognizer::State>().valueToKey(static_cast<int>(state)));
        });
        return true;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    // First, as the first pool settles the root. With no window to show it on, the log is where a refused
    // portable.txt is reported, and as in the GUI that comes before any Mudlet.ini is opened.
    void aPoolWithNoMainWindowReportsARefusedPortableMarkerBeforeOpeningMudletIni()
    {
#ifdef Q_OS_WIN
        QSKIP("QDir::homePath() does not follow HOME on Windows");
#endif
        QVERIFY2(!MudletApp::configRootSettled(), "something in this process settled the config root before the first pool");
        QTemporaryDir home;
        QVERIFY(home.isValid());
        const QString legacyDir = qsl("%1/.config/mudlet").arg(home.path());
        QVERIFY(QDir().mkpath(legacyDir));
        // Empty, which only beside the executable means "./portable"
        QFile marker(qsl("%1/portable.txt").arg(legacyDir));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        marker.close();
        const QString root = qsl("%1/mudlet").arg(mConfigDir.path());
        QFile ini(qsl("%1/Mudlet.ini").arg(root));
        QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
        ini.write("[General]\nmapCenterSmallAreas=true\n");
        ini.close();

        const QByteArray savedHome = qgetenv("HOME");
        qputenv("HOME", home.path().toUtf8());
        smReported = false;
        smStoreExistedWhenReported = false;
        smPreviousHandler = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
            if (message.contains(qsl("names no data directory, so"))) {
                smReported = true;
                smStoreExistedWhenReported = MudletApp::getQSettings() != nullptr;
            }
            smPreviousHandler(type, context, message);
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("portable.txt names no data directory\\.$")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("names no data directory, so \"%1\" is in use").arg(QRegularExpression::escape(root))));
        {
            HostManager pool;
            qInstallMessageHandler(smPreviousHandler);
            savedHome.isNull() ? qunsetenv("HOME") : qputenv("HOME", savedHome);

            QVERIFY2(smReported, "the refused portable.txt was never reported");
            QVERIFY2(!smStoreExistedWhenReported, "Mudlet.ini was opened before the refused portable.txt was reported");
            QVERIFY(MudletApp::configRootSettled());
            QVERIFY(!MudletApp::portableRootInUse());
            QCOMPARE(MudletApp::getMudletPath(enums::mainPath), root);
            QVERIFY2(MudletApp::getQSettings(), "a pool with no main window left the settings store unopened");
            QCOMPARE(MudletApp::getQSettings()->fileName(), qsl("%1/Mudlet.ini").arg(root));
            QVERIFY(MudletApp::getQSettings()->value(qsl("mapCenterSmallAreas")).toBool());
        }
        // The pool that settled the root is gone; the root and its store stay for the next
        QVERIFY(MudletApp::configRootSettled());
    }

    void theAccessorFollowsTheOneManagersLifetime()
    {
        QVERIFY2(!HostManager::self(), "an earlier test left its HostManager behind");
        {
            HostManager manager;
            QCOMPARE(HostManager::self(), &manager);
        }
        QVERIFY(!HostManager::self());
    }

    void aSecondManagerNeitherStealsTheAccessorNorClearsIt()
    {
        HostManager first;
        QCOMPARE(HostManager::self(), &first);
        Discord* const firstDiscord = Discord::self();
        QVERIFY(firstDiscord);
        TSpeechBridge* const firstSpeechBridge = TSpeechBridge::instance();
        QVERIFY(firstSpeechBridge);
        // Settling the root again would delete the store under whoever holds it, as the GUI's Updater does.
        // Guarded, as a store rebuilt at the freed address would compare equal.
        const QPointer<QSettings> store = MudletApp::getQSettings();
        QVERIFY(store);
        {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("a MudletMedia already exists")));
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("a FontManager already exists")));
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("a Discord already exists")));
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("a TSpeechBridge already exists")));
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("a HostManager already exists")));
            HostManager second;
            QCOMPARE(HostManager::self(), &first);
            QCOMPARE(Discord::self(), firstDiscord);
            QCOMPARE(TSpeechBridge::instance(), firstSpeechBridge);
            QVERIFY2(store && MudletApp::getQSettings() == store, "a second pool settled the config root again, deleting the first pool's settings store");
        }
        QCOMPARE(HostManager::self(), &first);
        QCOMPARE(Discord::self(), firstDiscord);
        QCOMPARE(TSpeechBridge::instance(), firstSpeechBridge);
    }

    // As an embedder's quit does with no main window: the bridge goes with this pool
    void aPoolGoingEndsItsProfilesSpeech()
    {
        const auto restoreFactory = installStandInFactory();
        {
            HostManager pool;
            QVERIFY2(pool.addHost(qsl("HostManagerAccessor-Speech"), QString(), QString(), QString()), "failed to put a profile in the pool");
            QVERIFY2(startListening(pool.getHost(qsl("HostManagerAccessor-Speech"))), "the profile did not start listening");
        }
        // Ended as the profile went, without running its scripts
        QCOMPARE(smEngineStates, QStringList({qsl("Ready")}));
        QCOMPARE(smScriptStates, QStringList({qsl("listening")}));
    }

    // Last, because it builds the application object - and with it the one
    // HostManager the rest of the process would otherwise have to work around.
    void theProfilesGoAwayWhileTheAccessorStillAnswers()
    {
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString profileName = qsl("HostManagerAccessor-Test");
        QVERIFY(HostManager::self());
        QVERIFY2(HostManager::self()->addHost(profileName, QString(), QString(), QString()), "failed to put a profile in the pool");
        Host* pHost = HostManager::self()->getHost(profileName);
        QVERIFY(pHost);

        // ~QObject() runs after ~Host()'s own body, so this is the latest point
        // in a profile's teardown - if the accessor still answers here it
        // answered for the whole of it.
        bool profileWasDestroyed = false;
        bool accessorAnsweredDuringTeardown = false;
        connect(pHost, &QObject::destroyed, qApp, [&profileWasDestroyed, &accessorAnsweredDuringTeardown]() {
            profileWasDestroyed = true;
            accessorAnsweredDuringTeardown = (HostManager::self() != nullptr);
        });

        // Listening as it goes, so the pool's teardown ends a session while the main window's members go
        const auto restoreFactory = installStandInFactory();
        QVERIFY2(startListening(pHost), "the profile did not start listening");
        // Run by the exit event the pool raises as it goes, once the main window's later members are gone
        static mudlet* sMainWindow = nullptr;
        static bool sConnectionChecked = false;
        static bool sStillConnected = false;
        sMainWindow = mudlet::self();
        sConnectionChecked = false;
        lua_register(pHost->getLuaInterpreter()->getLuaGlobalState(), "checkMainWindowConnection", [](lua_State*) -> int {
            sConnectionChecked = true;
            sStillConnected = QObject::disconnect(TSpeechBridge::instance(), &TSpeechBridge::microphoneOwnerChanged, sMainWindow, nullptr);
            return 0;
        });
        pHost->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("checkMainWindowConnection"));

        // ~mudlet() nulls its own accessor at the end of its body and the
        // HostManager member is destroyed after that, so this is also the point
        // where mudlet::self() has already gone
        delete mudlet::self();

        QVERIFY2(profileWasDestroyed, "the profile outlived the manager that held it, so this says nothing about the order");
        QVERIFY2(accessorAnsweredDuringTeardown, "the accessor was cleared before the profile pool it describes was emptied");
        QVERIFY2(sConnectionChecked, "the profile's exit handler never ran, so nothing was checked");
        QVERIFY2(!sStillConnected, "the main window was still taking the speech bridge's signal as its members went");
        // Ended as the profile went, without running its scripts while the main window's members go
        QCOMPARE(smEngineStates, QStringList({qsl("Ready")}));
        QCOMPARE(smScriptStates, QStringList({qsl("listening")}));
    }
};

#include "HostManagerAccessorTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostManagerAccessorTest)
