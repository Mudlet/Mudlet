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
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "SpeechRecognizer.h"
#include "SpeechRecognizerFactory.h"
#include "TLuaInterpreter.h"
#include "TSpeechBridge.h"
#include "discord.h"
#include "mudlet.h"

#include <QDir>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QScopeGuard>
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

    // What the listening profile's scripts and the engine are told from the start on
    inline static QStringList smScriptStates;
    inline static QStringList smEngineStates;

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
    void theAccessorFollowsTheOneManagersLifetime()
    {
        QVERIFY2(!HostManager::self(), "nothing in this process should have built a HostManager yet");
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
        }
        QCOMPARE(HostManager::self(), &first);
        QCOMPARE(Discord::self(), firstDiscord);
        QCOMPARE(TSpeechBridge::instance(), firstSpeechBridge);
    }

    // As an embedder's quit does with no main window: the bridge goes with this pool
    void aPoolGoingEndsItsProfilesSpeech()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QTemporaryDir configDir;
        QVERIFY(configDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(configDir.path())));
        const QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());
        const auto restoreXdg = qScopeGuard([&savedXdg]() {
            savedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", savedXdg);
        });
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
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QTemporaryDir configDir;
        QVERIFY(configDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(configDir.path())));
        const QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());

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

        savedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", savedXdg);

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
