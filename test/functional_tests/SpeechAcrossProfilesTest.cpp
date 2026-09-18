/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

/*
 * There is one speech recognizer for the whole application - one microphone,
 * one decoder - so at most one profile can be listening, and everything the
 * session produces belongs to the profile that started it rather than to
 * whichever one happens to be in front when a phrase lands. Those are the same
 * profile in the ordinary case, which is why routing by the second one went
 * unnoticed: it is wrong only when they differ, and they can only differ with
 * two profiles open.
 *
 * That is what puts this in a functional test rather than a spec, per the
 * testing note in CLAUDE.md: a spec runs inside one profile and cannot open a
 * second, so none of the cases here are reachable from one.
 *
 * No engine is installed in a test run, so none of this drives a real
 * recognizer. It does not need to - the ownership and the routing are core's,
 * and raiseSpeechEvent() is the seam every backend's results arrive through.
 *
 * What that does put out of reach is anything whose answer depends on the
 * recognizer's own state. Two rules here are covered by inspection rather than
 * by a case, because with no recognizer both sides of them read the same: a
 * claim being refused while the outgoing profile's phrase is still decoding,
 * and stt.listening() answering for the asking profile alone. A test for
 * either passes whether the rule holds or not, which is worse than none.
 *
 * Run with: ctest -R SpeechAcrossProfilesTest -V
 */

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QWidgetAction>
#include <QWindow>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "SpeechRecognizer.h"
#include "SpeechRecognizerFactory.h"
#include "TelnetServerStub.h"
#include "VoskRecognizer.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A stand-in engine, handed to the bridge through the factory the bridge
// itself calls, so everything around it - the signal wiring, the ownership
// bookkeeping, the routing of what it says - is the real thing. No CI runner
// has a speech engine installed, and the rules under test here are the
// bridge's own rather than any backend's.
class StandInRecognizer : public SpeechRecognizer
{
    Q_OBJECT

public:
    explicit StandInRecognizer(QObject* parent = nullptr)
    : SpeechRecognizer(parent)
    {
    }

    // The shape every backend's model load has: a session that was running
    // ends, and is reported, before the engine is ready again
    bool initialize(const QString&) override
    {
        endSessionForModelLoad();
        setState(State::Ready);
        return true;
    }
    QString currentLanguage() const override { return qsl("en"); }
    bool setLanguage(const QString&) override { return true; }
    QString backendName() const override { return qsl("StandIn"); }
    QString backendVersion() const override { return qsl("1.0"); }
    bool setSensitivity(Sensitivity) override { return true; }
    Sensitivity sensitivity() const override { return Sensitivity::Default; }
    QString modelPath() const override { return qsl("stand-in"); }

    // Parked where a backend sits while it finishes decoding the last phrase
    void beginProcessing() { setState(State::Processing); }

    // What a stop looks like on a backend that finalises the last phrase:
    // Processing while the decoder finishes, the phrase, then idle. The
    // handlers run inside the delivery, which is the whole point of it.
    void finishPhrase(const QString& text)
    {
        setState(State::Processing);
        emit finalResult(text);
        setState(State::Ready);
    }

protected:
    void doStartListening() override { setState(State::Listening); }
    void doStopListening() override { setState(State::Ready); }
    void doCancel() override { setState(State::Ready); }
};

class SpeechAcrossProfilesTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpFirstHost = nullptr;
    Host* mpSecondHost = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("SpeechAcrossProfiles-First");
    const QString mSecondHostname = qsl("SpeechAcrossProfiles-Second");

    bool mSystemEngineWins = false;

    // Where installStubEngine() puts the copy VoskRecognizer actually loads.
    static QString installedStubPath() { return QDir(VoskRecognizer::userLibraryPath()).filePath(QFileInfo(qsl(MUDLET_VOSK_STUB_LIBRARY)).fileName()); }

    // Puts the stand-in engine where librarySearchPaths() looks first, so a
    // recognizer can be built on a runner that has no speech engine at all -
    // which is every CI runner, and this test's own redirected config directory
    // even on a developer machine that has one installed.
    bool installStubEngine()
    {
        const QString destination = installedStubPath();
        if (!QDir().mkpath(VoskRecognizer::userLibraryPath())) {
            return false;
        }
        QFile::remove(destination);
        // Fresh probe: libraryAvailable() caches, and an earlier case may have
        // answered "no" before the file existed.
        VoskRecognizer::resetLibraryLoadState();
        VoskRecognizer::unloadLibraryByRequest(false);
        if (!QFile::copy(qsl(MUDLET_VOSK_STUB_LIBRARY), destination)) {
            return false;
        }
        // A copy that loads but exports nothing is the failure worth catching
        // here rather than three assertions later: it is what a Windows build
        // without WINDOWS_EXPORT_ALL_SYMBOLS produces.
        QLibrary installed(destination);
        if (!installed.load() || !installed.resolve("vosk_recognizer_set_words")) {
            // Balances the load: QLibrary refcounts and its destructor does not
            // unload, and Windows will not delete a module that is still mapped.
            installed.unload();
            return false;
        }
        installed.unload();
        return true;
    }

    Host* hostFor(const QString& profileName) const { return HostManager::self()->getHost(profileName); }

    // Hands the bridge a stand-in engine through the factory the bridge itself
    // calls, so the wiring under test is the wiring Mudlet ships.
    StandInRecognizer* installStandInEngine()
    {
        // The stand-in library as well as the stand-in engine. Every Lua
        // setter asks the bridge for an engine before it does anything, naming
        // the backend that could be built here - and with no engine library at
        // all that is the built-in macOS one, which is a different backend from
        // whatever is in place and so rebuilds it, handing the call a
        // recognizer with no model. Making one engine installable keeps those
        // calls on "whatever is already there".
        if (!installStubEngine()) {
            return nullptr;
        }
        SpeechRecognizerFactory::setFactoryOverride([](QObject* parent) -> SpeechRecognizer* {
            return new StandInRecognizer(parent);
        });
        // Named rather than Auto, and both names tried: initSpeechRecognition()
        // keeps the engine it has for Auto and for the backend that engine
        // already is, so a case running after another one left a recognizer in
        // place would otherwise drive that one instead of this stand-in.
        for (const SpeechRecognizerFactory::Backend backend : {SpeechRecognizerFactory::Backend::Sherpa, SpeechRecognizerFactory::Backend::Vosk}) {
            mudlet::self()->initSpeechRecognition(backend);
            if (auto* pStandIn = qobject_cast<StandInRecognizer*>(mudlet::self()->speechRecognizer())) {
                return pStandIn;
            }
        }
        return nullptr;
    }

    // Takes the stand-in back out of the way of the cases that follow: the
    // engine stays where it is, closed, and the factory answers for itself
    // again.
    void retireStandInEngine()
    {
        SpeechRecognizerFactory::setFactoryOverride(nullptr);
        if (auto* pRecognizer = mudlet::self()->speechRecognizer()) {
            pRecognizer->releaseResources();
        }
        mudlet::self()->releaseMicrophone();
        QTest::qWait(50ms);
    }

    // The connection dialog can only be driven once: with the main window up it
    // never takes activation again, so the second profile is opened the way a
    // player opens one from inside a running Mudlet instead.
    bool provisionProfileOnDisk(const QString& profileName) const
    {
        return QDir().mkpath(MudletPaths::getMudletPath(enums::profileHomePath, profileName)) && MudletPaths::writeProfileData(profileName, qsl("url"), mLocalhost).first
               && MudletPaths::writeProfileData(profileName, qsl("port"), mPort).first;
    }

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // Returns the Lua error, or a null QString when the chunk ran
    QString runLua(Host* pHost, const QString& code) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    QString luaGlobalString(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const char* value = lua_tostring(L, -1);
        const QString result = value ? QString::fromUtf8(value) : QString();
        lua_pop(L, 1);
        return result;
    }

    // Records the argument of one sysSTT* event into a global of this profile's
    // own, so that "which profile heard it" is a question each Lua state
    // answers for itself rather than one the test has to infer.
    void listenFor(Host* pHost, const QString& eventName, const QString& globalName) const
    {
        const QString code = qsl("%1 = nil registerAnonymousEventHandler(\"%2\", function(_, value) %1 = value end)").arg(globalName, eventName);
        QVERIFY2(runLua(pHost, code).isNull(), qPrintable(qsl("could not arm a handler for %1").arg(eventName)));
    }

    bool luaGlobalBoolean(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const bool value = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return value;
    }

    int luaGlobalNumber(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // The id a Lua call returned, or -1 when the command was refused
    int addCommand(Host* pHost, const QString& fields) const
    {
        const QString error = runLua(pHost, qsl("_addonId = addCommand{%1} or -1").arg(fields));
        if (!error.isNull()) {
            qWarning() << "addCommand:" << error;
            return -1;
        }
        return luaGlobalNumber(pHost, qsl("_addonId"));
    }

    // The button a command placed, searched for in one window only - which is
    // the whole question here, since the bug being pinned is a button appearing
    // in a window that is not its profile's.
    QToolButton* buttonIn(QWidget* pContainer, const QString& name) const { return pContainer->findChild<QToolButton*>(qsl("addon_%1").arg(name), Qt::FindChildrenRecursively); }

    // The toolbar entry a command placed. Asked for rather than the button's own
    // isVisible(), which in an offscreen run is false for every widget in a
    // window that was never shown, and rather than isHidden(), which a toolbar
    // that is itself hidden sets on the buttons inside it. The action's
    // visibility is the command's own answer either way.
    QAction* toolbarEntryIn(QWidget* pContainer, const QString& name) const
    {
        QToolButton* pButton = buttonIn(pContainer, name);
        if (!pButton) {
            return nullptr;
        }
        for (QToolBar* pToolBar : pContainer->findChildren<QToolBar*>()) {
            for (QAction* pAction : pToolBar->actions()) {
                auto* pWidgetAction = qobject_cast<QWidgetAction*>(pAction);
                if (pWidgetAction && pWidgetAction->defaultWidget() == pButton) {
                    return pAction;
                }
            }
        }
        return nullptr;
    }

    QAction* menuItemIn(QWidget* pContainer, const QString& name) const
    {
        for (QAction* pAction : pContainer->findChildren<QAction*>()) {
            if (pAction->text() == name) {
                return pAction;
            }
        }
        return nullptr;
    }

    void deleteProfileDirectory(const QString& profileName) const
    {
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        // Decided once, before anything has installed or loaded a stub.
        // loadVoskLibrary() asks QLibrary for the bare name "vosk" and only
        // falls back to librarySearchPaths() when that fails, so a machine with
        // libvosk on the loader's own path never reaches the copy installed
        // below. The probe has to happen while nothing is mapped: dlopen() and
        // LoadLibrary() both answer a bare name out of what is already loaded.
        QLibrary bare(qsl("vosk"));
        mSystemEngineWins = bare.load();
        if (mSystemEngineWins) {
            bare.unload();
        }

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);

        mpFirstHost = TestProfile::create(mFirstHostname, mLocalhost, mPort);
        QVERIFY2(mpFirstHost, "the first profile did not load");

        QVERIFY(provisionProfileOnDisk(mSecondHostname));
        QVERIFY2(runLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(mSecondHostname)).isNull(), "the second profile could not be loaded");
        QTest::qWait(500ms);
        mpSecondHost = hostFor(mSecondHostname);
        QVERIFY2(mpSecondHost, "the second profile did not open");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The main toolbar is hidden by default, and a toolbar-only command is
    // refused while it is - which would leave every placement case below
    // asserting against a command that was never created.
    void init() { mudlet::self()->setToolBarVisibility(enums::visibleAlways); }

    // No test may leave the microphone claimed or a profile detached: the owner
    // decides where the next test's events land, and a detached profile changes
    // which window its commands are in. Done here rather than at the end of
    // each case because a QVERIFY that fails leaves the rest of its body unrun -
    // so tidying inline means one real failure arrives as several.
    void cleanup()
    {
        mudlet::self()->releaseMicrophone();
        if (mudlet::self()->getDetachedWindows().contains(mSecondHostname)) {
            mudlet::self()->slot_tabReattachRequested(mSecondHostname);
            QTest::qWait(200ms);
        }
        mudlet::self()->activateWindow();
    }

    // The one speech event that is not a session's own. What a backend can do
    // is a property of the engine, and every profile reads the same answer back
    // from stt.getInfo() - so announcing to the profile holding the microphone
    // alone would move what the others read with nothing said to them, which is
    // #10760 one profile further out.
    //
    // Declared first among these cases because the change it turns on is a
    // recognizer coming into existence, and that happens once per process.
    void test_aCapabilityChangeReachesEveryProfile()
    {
        if (mSystemEngineWins) {
            QSKIP("libvosk answers the bare name here, so the loader would reach it before the stand-in this case installs");
        }
        if (mudlet::self()->speechRecognizer()) {
            QSKIP("a recognizer already exists, so its arrival - the change this case is about - is already behind us");
        }
        QVERIFY2(installStubEngine(), "the stand-in engine could not be installed, so no recognizer can be built here");

        listenFor(mpFirstHost, qsl("sysSTTCapabilitiesChanged"), qsl("_capsFirst"));
        listenFor(mpSecondHost, qsl("sysSTTCapabilitiesChanged"), qsl("_capsSecond"));

        // Held deliberately: the owner is what every other speech event routes
        // by, so a broadcast that had quietly gone back to owner-routing would
        // still look right with nobody holding the microphone.
        mudlet::self()->claimMicrophoneFor(mpFirstHost);

        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Vosk);
        QVERIFY2(mudlet::self()->speechRecognizer(), "the stand-in engine was installed but no recognizer was built from it");

        const QString owner = luaGlobalString(mpFirstHost, qsl("_capsFirst"));
        const QString other = luaGlobalString(mpSecondHost, qsl("_capsSecond"));
        QVERIFY2(!owner.isEmpty(), "the profile holding the microphone was not told the capabilities changed");
        QVERIFY2(!other.isEmpty(), "a profile holding no microphone reads the same capabilities and was not told they changed");
        QCOMPARE(other, owner);
    }

    // The case the whole ownership model exists for. Listening starts in one
    // profile, the player tabs to the other, and the phrase they finish saying
    // belongs to the game they said it to - not to the one now in front.
    void test_aResultGoesToTheProfileHoldingTheMicrophone()
    {
        listenFor(mpFirstHost, qsl("sysSTTResult"), qsl("_heardFirst"));
        listenFor(mpSecondHost, qsl("sysSTTResult"), qsl("_heardSecond"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->activateProfile(mpSecondHost);
        QCOMPARE(mudlet::self()->getActiveHost(), mpSecondHost);

        mudlet::self()->raiseSpeechEvent(qsl("sysSTTResult"), qsl("kill hound"));

        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_heardFirst")), qsl("kill hound"));
        QVERIFY2(luaGlobalString(mpSecondHost, qsl("_heardSecond")).isEmpty(), "the phrase was delivered to the profile in front rather than to the one that was listening");
    }

    // The other half of the same rule: with no session running there is no
    // owner to prefer, and a refusal or a capability change still has to reach
    // somebody. The profile in front is the right answer then, and only then.
    void test_withNoOwnerAnEventGoesToTheProfileInFront()
    {
        listenFor(mpFirstHost, qsl("sysSTTError"), qsl("_faultFirst"));
        listenFor(mpSecondHost, qsl("sysSTTError"), qsl("_faultSecond"));

        mudlet::self()->releaseMicrophone();
        mudlet::self()->activateProfile(mpSecondHost);

        mudlet::self()->raiseSpeechEvent(qsl("sysSTTError"), qsl("no model"));

        QCOMPARE(luaGlobalString(mpSecondHost, qsl("_faultSecond")), qsl("no model"));
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_faultFirst")).isEmpty(), "an event with no session running went somewhere other than the profile in front");
    }

    // A second profile asking to listen is a handover, and the profile that
    // loses the microphone is told by name. Nothing else on its screen would
    // say why it stopped: its own button simply goes quiet.
    void test_takingTheMicrophoneTellsTheProfileThatLostIt()
    {
        listenFor(mpFirstHost, qsl("sysSTTHandover"), qsl("_handoverFirst"));
        listenFor(mpSecondHost, qsl("sysSTTHandover"), qsl("_handoverSecond"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->claimMicrophoneFor(mpSecondHost);

        QCOMPARE(mudlet::self()->microphoneOwner(), mpSecondHost);
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_handoverFirst")), mSecondHostname);
        QVERIFY2(luaGlobalString(mpSecondHost, qsl("_handoverSecond")).isEmpty(), "the profile that took the microphone was told it had lost one");
    }

    // Re-asking for a microphone a profile already holds is not a handover, and
    // announcing one would have every repeated stt.start() tell the profile it
    // had lost the session it is still running.
    void test_askingTwiceFromOneProfileAnnouncesNothing()
    {
        listenFor(mpFirstHost, qsl("sysSTTHandover"), qsl("_handoverAgain"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        // Answers true without taking anything, which is what lets a caller
        // tell "I hold it" apart from "I could not have it" and give back only
        // what its own call actually took.
        QVERIFY2(mudlet::self()->claimMicrophoneFor(mpFirstHost), "a profile could not re-claim the microphone it already held");

        QCOMPARE(mudlet::self()->microphoneOwner(), mpFirstHost);
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_handoverAgain")).isEmpty(), "a profile that kept the microphone was told it had lost it");
    }

    // Releasing is what the end of a session does, and it has to put routing
    // back to the profile in front rather than leaving the last owner latched.
    void test_releasingHandsRoutingBackToTheProfileInFront()
    {
        listenFor(mpFirstHost, qsl("sysSTTStateChanged"), qsl("_stateFirst"));
        listenFor(mpSecondHost, qsl("sysSTTStateChanged"), qsl("_stateSecond"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->activateProfile(mpSecondHost);
        mudlet::self()->releaseMicrophone();

        mudlet::self()->raiseSpeechEvent(qsl("sysSTTStateChanged"), qsl("ready"));

        QCOMPARE(luaGlobalString(mpSecondHost, qsl("_stateSecond")), qsl("ready"));
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_stateFirst")).isEmpty(), "a released microphone left its old owner still receiving the session's events");
    }

    // The duplicate the player reported: every open profile placed its command
    // in the main window's toolbar, so two games meant two identical buttons
    // with nothing but a tooltip to tell them apart.
    void test_onlyTheShownProfilesCommandIsOnTheToolbar()
    {
        const int firstId = addCommand(mpFirstHost, qsl("name = \"SpeechFirst\", surfaces = \"toolbar\""));
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechSecond\", surfaces = \"toolbar\""));
        QVERIFY(firstId > 0 && secondId > 0);

        mudlet::self()->activateProfile(mpFirstHost);
        QAction* pFirstEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechFirst"));
        QAction* pSecondEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechSecond"));
        QVERIFY2(pFirstEntry && pSecondEntry, "both commands should exist in the main window, whatever their visibility");
        QVERIFY2(pFirstEntry->isVisible(), "the shown profile's button is not on the toolbar");
        QVERIFY2(!pSecondEntry->isVisible(), "a profile that is not being shown still has a button on the toolbar");

        mudlet::self()->activateProfile(mpSecondHost);
        QVERIFY2(!pFirstEntry->isVisible(), "switching profiles left the old profile's button behind");
        QVERIFY2(pSecondEntry->isVisible(), "switching profiles did not bring the new profile's button out");

        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(firstId));
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The menu half goes with it, and takes its shortcut: a key that raises
    // another game's event while you look at this one is the same mistake as
    // a button that does.
    void test_aHiddenProfilesMenuItemIsHiddenToo()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"MenuSecond\", menuPath = \"Speech\", surfaces = \"menu\""));
        QVERIFY(secondId > 0);

        mudlet::self()->activateProfile(mpSecondHost);
        QAction* pItem = menuItemIn(mudlet::self(), qsl("MenuSecond"));
        QVERIFY2(pItem, "the menu item was never placed");
        QVERIFY2(pItem->isVisible(), "the shown profile's menu item is not on the menu");

        mudlet::self()->activateProfile(mpFirstHost);
        QVERIFY2(!pItem->isVisible(), "another profile's menu item is still on the menu");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // A detached profile's button used to stay in the main window, which is the
    // one window it certainly does not belong in.
    void test_detachingTakesTheCommandToItsNewWindow()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechMoved\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        QVERIFY2(buttonIn(mudlet::self(), qsl("SpeechMoved")), "the command was not placed in the main window to begin with");

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QVERIFY2(!buttonIn(mudlet::self(), qsl("SpeechMoved")), "the command stayed in the main window after its profile was detached");
        QVERIFY2(buttonIn(pWindow, qsl("SpeechMoved")), "the command did not arrive in the detached window");
        QAction* pMovedEntry = toolbarEntryIn(pWindow, qsl("SpeechMoved"));
        QVERIFY2(pMovedEntry && pMovedEntry->isVisible(), "the command arrived in the detached window but is not shown, though that window shows its profile");

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        QVERIFY2(buttonIn(mudlet::self(), qsl("SpeechMoved")), "reattaching did not bring the command back to the main window");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // A detached window sizes its own toolbar from the same change the main
    // window does, and a command's button is sized from the toolbar of the
    // window it is in - so the two have to happen in that order. Read the other
    // way round, a detached button keeps the size its window has just stopped
    // using, and only catches up at the next change that never comes.
    void test_aDetachedButtonFollowsAnIconSizeChangeAtOnce()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechSized\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);
        QToolButton* pButton = buttonIn(pWindow, qsl("SpeechSized"));
        QVERIFY2(pButton, "the command did not arrive in the detached window");

        const int startingSize = mudlet::self()->mToolbarIconSize;
        const int changedSize = (startingSize == 3) ? 2 : 3;
        mudlet::self()->setToolBarIconSize(changedSize);
        QTest::qWait(100ms);

        const QSize buttonSize = pButton->iconSize();
        const QSize expected = QSize(changedSize * 8, changedSize * 8);

        mudlet::self()->setToolBarIconSize(startingSize);
        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));

        QCOMPARE(buttonSize, expected);
    }

    // The widgets are rebuilt on a move, so everything the package set has to
    // be re-applied to them - it lives on the command, not on the button.
    void test_aMovedCommandKeepsWhatThePackageSet()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechState\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        runLua(mpSecondHost, qsl("setCommandChecked(%1, true)").arg(secondId));
        runLua(mpSecondHost, qsl("setCommandTooltip(%1, \"listening in StickMUD\")").arg(secondId));
        runLua(mpSecondHost, qsl("disableCommand(%1)").arg(secondId));

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QToolButton* pMoved = buttonIn(pWindow, qsl("SpeechState"));
        QVERIFY2(pMoved, "the command did not arrive in the detached window");
        QVERIFY2(pMoved->isChecked(), "a checked command came out of the move unchecked");
        QVERIFY2(pMoved->toolTip().contains(qsl("listening in StickMUD")), qPrintable(qsl("the tooltip did not survive the move: %1").arg(pMoved->toolTip())));
        QVERIFY2(!pMoved->isEnabled(), "a disabled command came out of the move enabled");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // What the player asked for: a live microphone they can always see and stop,
    // without the two idle buttons that made them ask. A pinned command is shown
    // in the window they are in even while it shows another profile - and only
    // one command is ever pinned, the one that is actually doing something.
    void test_aPinnedCommandIsShownWhileAnotherProfileIsInFront()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechPinned\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);

        mudlet::self()->activateProfile(mpFirstHost);
        QAction* pEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechPinned"));
        QVERIFY2(pEntry, "the command was not placed at all");
        QVERIFY2(!pEntry->isVisible(), "an unpinned command of another profile is on the toolbar");

        QVERIFY(runLua(mpSecondHost, qsl("setCommandPinned(%1, true)").arg(secondId)).isNull());
        QVERIFY2(pEntry->isVisible(), "a pinned command is not shown while another profile is in front");

        QVERIFY(runLua(mpSecondHost, qsl("setCommandPinned(%1, false)").arg(secondId)).isNull());
        QVERIFY2(!pEntry->isVisible(), "unpinning did not put the command back with its own profile");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // Pinning is the profile's own business, like every other command operation:
    // an id belonging to somebody else answers as an unknown one does.
    void test_pinningIsRefusedForAnotherProfilesCommand()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechNotYours\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);

        QVERIFY(runLua(mpFirstHost, qsl("_pinnedOther = setCommandPinned(%1, true)").arg(secondId)).isNull());
        QVERIFY2(!luaGlobalBoolean(mpFirstHost, qsl("_pinnedOther")), "a profile pinned another profile's command");

        QVERIFY(runLua(mpFirstHost, qsl("_pinnedUnknown = setCommandPinned(999999, true)")).isNull());
        QVERIFY2(!luaGlobalBoolean(mpFirstHost, qsl("_pinnedUnknown")), "an unknown id was accepted");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The one signal that survives the window being minimised or behind
    // something else, which is exactly the case "stt focus keep" creates.
    void test_theWindowTitleSaysWhenItsProfileHasTheMicrophone()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        const QString quiet = mudlet::self()->windowTitle();
        QVERIFY2(!quiet.contains(qsl("listening")), "the title claimed a microphone before one was open");

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        QVERIFY2(mudlet::self()->windowTitle().contains(qsl("listening")), qPrintable(qsl("the title does not say the microphone is open: %1").arg(mudlet::self()->windowTitle())));

        mudlet::self()->releaseMicrophone();
        QCOMPARE(mudlet::self()->windowTitle(), quiet);
    }

    // A profile in another window holding the microphone is not this window's
    // news, or every window would claim the one microphone at once.
    void test_onlyTheOwningWindowsTitleIsMarked()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        mudlet::self()->claimMicrophoneFor(mpSecondHost);
        QVERIFY2(pWindow->windowTitle().contains(qsl("listening")), qPrintable(qsl("the detached window holding the microphone does not say so: %1").arg(pWindow->windowTitle())));
        QVERIFY2(!mudlet::self()->windowTitle().contains(qsl("listening")), "the main window claimed a microphone belonging to a profile in another window");

        mudlet::self()->releaseMicrophone();
    }

    // The same move into a window that is already open, which is what the second
    // window onwards gets and goes through a different path to the one that
    // builds a window. The main window has to stop claiming a microphone it no
    // longer draws: the marker in two places at once says two are open.
    void test_movingTheListeningProfileIntoAnOpenWindowUnmarksTheMainTitle()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        QVERIFY2(mudlet::self()->windowTitle().contains(qsl("listening")), qPrintable(qsl("the main window did not mark the microphone its own profile holds: %1").arg(mudlet::self()->windowTitle())));

        mudlet::self()->slot_profileDetachToWindow(mFirstHostname, pWindow);
        QTest::qWait(200ms);

        const QString mainTitle = mudlet::self()->windowTitle();
        const QString movedIntoTitle = pWindow->windowTitle();

        mudlet::self()->releaseMicrophone();
        mudlet::self()->slot_tabReattachRequested(mFirstHostname);
        QTest::qWait(200ms);
        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        mudlet::self()->activateProfile(mpFirstHost);

        QVERIFY2(!mainTitle.contains(qsl("listening")), qPrintable(qsl("the main window went on claiming a microphone that had left it: %1").arg(mainTitle)));
        QVERIFY2(movedIntoTitle.contains(qsl("listening")), qPrintable(qsl("the window the listening profile moved into does not say so: %1").arg(movedIntoTitle)));
    }

    // Dragging a profile out of the main window while it is listening moves its
    // control to the new window and the marker with it. The claim itself must
    // not move: the session is still running and still belongs to that profile,
    // whichever window is now drawing it.
    void test_detachingAProfileThatHoldsTheMicrophoneCarriesBothWithIt()
    {
        // Deliberately not pinned: a pinned command follows the player rather
        // than its profile, and which window has focus after a detach is the
        // window manager's business - not something to assert offscreen. Where
        // a pinned one goes is covered by the pinning case above.
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechLive\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        mudlet::self()->claimMicrophoneFor(mpSecondHost);

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QCOMPARE(mudlet::self()->microphoneOwner(), mpSecondHost);
        QVERIFY2(pWindow->windowTitle().contains(qsl("listening")), "the detached window does not say it took the open microphone with it");
        QVERIFY2(buttonIn(pWindow, qsl("SpeechLive")), "the live command did not follow its profile out of the main window");

        QVERIFY2(!buttonIn(mudlet::self(), qsl("SpeechLive")), "the command stayed in the main window while its listening profile left");

        mudlet::self()->releaseMicrophone();
        QVERIFY2(!pWindow->windowTitle().contains(qsl("listening")), "the marker outlived the session it was describing");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // Pinning is for a control the player must be able to reach from wherever
    // they are, so it has to follow them between Mudlet's own windows - not
    // only when Mudlet as a whole comes to the front.
    void test_aPinnedCommandFollowsThePlayerBetweenWindows()
    {
        const int firstId = addCommand(mpFirstHost, qsl("name = \"SpeechFollow\", surfaces = \"toolbar\""));
        QVERIFY(firstId > 0);

        mudlet::self()->activateProfile(mpFirstHost);
        mudlet::self()->activateWindow();
        QVERIFY(runLua(mpFirstHost, qsl("setCommandPinned(%1, true)").arg(firstId)).isNull());
        QVERIFY2(buttonIn(mudlet::self(), qsl("SpeechFollow")), "the pinned command should start in the window the player is in");

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);
        pWindow->activateWindow();
        QVERIFY2(QTest::qWaitFor(
                         [pWindow]() {
                             return QApplication::activeWindow() == pWindow;
                         },
                         2000),
                 "the detached window never became the active one, so this cannot test what follows focus");

        QVERIFY2(buttonIn(pWindow, qsl("SpeechFollow")), "a pinned command did not follow the player into the window they moved to");

        QVERIFY(runLua(mpFirstHost, qsl("setCommandPinned(%1, false)").arg(firstId)).isNull());
        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(firstId));
    }

    // A pinned command visits windows its package never asked about, and comes
    // home to one the profile has gone on using in the meantime. Here the
    // profile put a command where the pinned one's menu path used to be - it
    // could, because the submenu left with the pinned command - and the pinned
    // command has to arrive anyway. A label being both a command and a submenu
    // is refused when a package asks for it, which is the moment it can choose
    // another path; taking a placed command's menu item away instead just
    // leaves it with no way back.
    void test_aPinnedCommandComingHomeKeepsItsMenuItem()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        mudlet::self()->activateWindow();

        const int pinnedId = addCommand(mpFirstHost, qsl("name = \"SpeechPinnedItem\", menuPath = \"SpeechPath\""));
        QVERIFY(pinnedId > 0);
        QVERIFY(runLua(mpFirstHost, qsl("setCommandPinned(%1, true)").arg(pinnedId)).isNull());

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);
        pWindow->activateWindow();
        QVERIFY2(QTest::qWaitFor(
                         [pWindow]() {
                             return QApplication::activeWindow() == pWindow;
                         },
                         2000),
                 "the detached window never became the active one, so the pinned command never left home");
        QVERIFY2(menuItemIn(pWindow, qsl("SpeechPinnedItem")), "the pinned command did not follow the player into the other window");

        // The submenu went with it, so this name is free in the window the
        // pinned command came from - and taken by the time it returns
        const int clashingId = addCommand(mpFirstHost, qsl("name = \"SpeechPath\""));
        QVERIFY2(clashingId > 0, "a command could not take the name the pinned command's menu path had used");

        mudlet::self()->activateWindow();
        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return QApplication::activeWindow() == mudlet::self();
                         },
                         2000),
                 "the main window never became the active one, so the pinned command never came home");

        const bool itemCameBack = menuItemIn(mudlet::self(), qsl("SpeechPinnedItem")) != nullptr;

        runLua(mpFirstHost, qsl("setCommandPinned(%1, false)").arg(pinnedId));
        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(pinnedId));
        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(clashingId));
        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);

        QVERIFY2(itemCameBack, "a pinned command came home to a window holding a command named like its menu path and lost its menu item");
    }

    // "ready" is where a package waits to start listening, so a handler for it
    // calling stt.start() is the ordinary shape rather than an odd one. The
    // session it starts belongs to the profile that started it: the release
    // that ends the old session must not carry off the claim the new one just
    // made, or the microphone is open with nobody holding it and
    // stt.listening() answers no to the profile actually listening.
    void test_aSessionStartedFromAReadyHandlerHasAnOwner()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        StandInRecognizer* pEngine = installStandInEngine();
        QVERIFY2(pEngine, "the stand-in engine was not installed");

        QVERIFY(runLua(mpFirstHost,
                       qsl("_sttStarted = false\n"
                           "_sttReadyHandler = registerAnonymousEventHandler('sysSTTStateChanged', function(_, state)\n"
                           "  if state == 'ready' and not _sttStarted then _sttStarted = true; stt.start() end\n"
                           "end)"))
                        .isNull());

        pEngine->initialize(QString());
        QTest::qWait(50ms);

        const Host* pOwner = mudlet::self()->microphoneOwner();
        const bool engineListening = pEngine->listening();
        runLua(mpFirstHost, qsl("_sttLuaListening = stt.listening()"));
        const bool luaSaysListening = luaGlobalBoolean(mpFirstHost, qsl("_sttLuaListening"));

        runLua(mpFirstHost, qsl("killAnonymousEventHandler(_sttReadyHandler)"));
        retireStandInEngine();

        QVERIFY2(engineListening, "the handler's stt.start() did not reach a listening engine, so this case proves nothing");
        QCOMPARE(pOwner, mpFirstHost);
        QVERIFY2(luaSaysListening, "the profile that started the session was told it was not listening");
    }

    // Loading a model ends whatever session is running, and the profile that
    // was speaking is the one that needs to know: the phrase it was in the
    // middle of is gone. The engine settles its state before it says so, and
    // the release rides on that state - so without care the sentence arrives at
    // whichever profile happens to be in front, telling a game that never
    // spoke that it lost words, and leaving the game that did with silence.
    void test_theProfileThatLosesASessionToAModelLoadIsTheOneTold()
    {
        mudlet::self()->activateProfile(mpSecondHost);
        StandInRecognizer* pEngine = installStandInEngine();
        QVERIFY2(pEngine, "the stand-in engine was not installed");
        pEngine->initialize(QString());

        QVERIFY(runLua(mpFirstHost, qsl("_sttFirstErrors = {}\n_sttFirstHandler = registerAnonymousEventHandler('sysSTTError', function(_, message) table.insert(_sttFirstErrors, message) end)"))
                        .isNull());
        QVERIFY(runLua(mpSecondHost, qsl("_sttSecondErrors = {}\n_sttSecondHandler = registerAnonymousEventHandler('sysSTTError', function(_, message) table.insert(_sttSecondErrors, message) end)"))
                        .isNull());

        QVERIFY(runLua(mpSecondHost, qsl("_sttStartedSecond = stt.start()")).isNull());
        QVERIFY2(luaGlobalBoolean(mpSecondHost, qsl("_sttStartedSecond")), "the second profile could not start a session");
        QCOMPARE(mudlet::self()->microphoneOwner(), mpSecondHost);

        // The player has moved on to the other game, which is what makes the
        // two answers differ at all
        mudlet::self()->activateProfile(mpFirstHost);
        pEngine->initialize(QString());
        QTest::qWait(50ms);

        runLua(mpFirstHost, qsl("_sttFirstSaid = table.concat(_sttFirstErrors, '|')"));
        runLua(mpSecondHost, qsl("_sttSecondSaid = table.concat(_sttSecondErrors, '|')"));
        const QString heardByFirst = luaGlobalString(mpFirstHost, qsl("_sttFirstSaid"));
        const QString heardBySecond = luaGlobalString(mpSecondHost, qsl("_sttSecondSaid"));

        runLua(mpFirstHost, qsl("killAnonymousEventHandler(_sttFirstHandler)"));
        runLua(mpSecondHost, qsl("killAnonymousEventHandler(_sttSecondHandler)"));
        retireStandInEngine();

        QVERIFY2(heardBySecond.contains(qsl("stopped the listening session")), qPrintable(qsl("the profile that lost its session was not told: \"%1\"").arg(heardBySecond)));
        QVERIFY2(!heardByFirst.contains(qsl("stopped the listening session")), qPrintable(qsl("a profile that was not listening was told it had lost a session: \"%1\"").arg(heardByFirst)));
    }

    // Closing from a sysSTTResult handler is closing because of the phrase that
    // handler was just handed. The engine still reads as Processing while it
    // runs, so the close used to answer with the one outcome that did not
    // happen - that the phrase was lost - to the package holding it.
    void test_closingFromAResultHandlerIsNotToldThePhraseWasLost()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        StandInRecognizer* pEngine = installStandInEngine();
        QVERIFY2(pEngine, "the stand-in engine was not installed");
        pEngine->initialize(QString());

        QVERIFY(runLua(mpFirstHost, qsl("_sttCloseErrors = {}\n_sttCloseHandler = registerAnonymousEventHandler('sysSTTError', function(_, message) table.insert(_sttCloseErrors, message) end)"))
                        .isNull());
        QVERIFY(runLua(mpFirstHost, qsl("_sttResultHandler = registerAnonymousEventHandler('sysSTTResult', function(_, text) _sttHeard = text; stt.close() end)")).isNull());

        QVERIFY(runLua(mpFirstHost, qsl("_sttStartedFirst = stt.start()")).isNull());
        QVERIFY2(luaGlobalBoolean(mpFirstHost, qsl("_sttStartedFirst")), "the profile could not start a session");

        pEngine->finishPhrase(qsl("kill hound"));
        QTest::qWait(50ms);

        runLua(mpFirstHost, qsl("_sttCloseSaid = table.concat(_sttCloseErrors, '|')"));
        const QString heard = luaGlobalString(mpFirstHost, qsl("_sttHeard"));
        const QString said = luaGlobalString(mpFirstHost, qsl("_sttCloseSaid"));

        runLua(mpFirstHost, qsl("killAnonymousEventHandler(_sttCloseHandler)"));
        runLua(mpFirstHost, qsl("killAnonymousEventHandler(_sttResultHandler)"));
        retireStandInEngine();

        QCOMPARE(heard, qsl("kill hound"));
        QVERIFY2(!said.contains(qsl("that phrase is lost")), qPrintable(qsl("the phrase that was just delivered was reported lost: \"%1\"").arg(said)));
    }

    // A library sitting broken in the folder Mudlet told the player to install
    // into is not a library that is missing, and the reason it would not load
    // is the only sentence that says what to do about it. Reported from the
    // path that actually holds a file: the loader walks several, and what the
    // last attempt leaves behind is "No such file" against one the player has
    // nothing at - an answer that sends them looking in the wrong place for a
    // file they do not have.
    void test_aBrokenLibraryIsReportedFromThePathThatHoldsIt()
    {
        if (mSystemEngineWins) {
            QSKIP("libvosk answers the bare name here, so the loader reaches it before anything this case installs");
        }

        const QString broken = installedStubPath();
        QVERIFY(QDir().mkpath(VoskRecognizer::userLibraryPath()));
        QFile::remove(broken);
        QFile file(broken);
        QVERIFY2(file.open(QIODevice::WriteOnly), "the broken library could not be written");
        file.write("this is not a shared library");
        file.close();

        VoskRecognizer::resetLibraryLoadState();
        VoskRecognizer::unloadLibraryByRequest(false);

        const bool loaded = VoskRecognizer::libraryAvailable();
        const QString reason = VoskRecognizer::libraryLoadError();

        // Back to nothing installed, and probed afresh, so the next case finds
        // the state this one started from
        QFile::remove(broken);
        VoskRecognizer::resetLibraryLoadState();

        QVERIFY2(!loaded, "a file of nonsense was accepted as a speech engine");
        QVERIFY2(!reason.isEmpty(), "a library that is installed and will not load said nothing about why");
        QVERIFY2(reason.contains(broken), qPrintable(qsl("the reason names a path other than the file that would not load: \"%1\"").arg(reason)));
    }

    // A session that ends gives the microphone back. Held until then, and by
    // the profile that started it, so what the engine produced on the way out
    // still reaches the game that was speaking - but a claim outliving its
    // session would send the next profile's results to a game that had stopped.
    void test_endingASessionGivesTheMicrophoneBack()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        StandInRecognizer* pEngine = installStandInEngine();
        QVERIFY2(pEngine, "the stand-in engine was not installed");
        pEngine->initialize(QString());

        QVERIFY(runLua(mpFirstHost, qsl("_sttStartedOwner = stt.start()")).isNull());
        QVERIFY2(luaGlobalBoolean(mpFirstHost, qsl("_sttStartedOwner")), "the session did not start");
        QCOMPARE(mudlet::self()->microphoneOwner(), mpFirstHost);

        runLua(mpFirstHost, qsl("stt.stop()"));
        QTest::qWait(50ms);

        const Host* pOwnerAfter = mudlet::self()->microphoneOwner();
        retireStandInEngine();

        QVERIFY2(pOwnerAfter == nullptr, "the microphone was still held after the session that claimed it ended");
    }

    // Stopping is as much a part of owning a session as starting was. A profile
    // that holds nothing has nothing to stop, and reaching across to end
    // another game's session would leave that game with a bare state change -
    // the ambiguity sysSTTHandover exists to remove, through another door.
    void test_stoppingIsRefusedFromAProfileThatHoldsNoSession()
    {
        mudlet::self()->activateProfile(mpSecondHost);
        StandInRecognizer* pEngine = installStandInEngine();
        QVERIFY2(pEngine, "the stand-in engine was not installed");
        pEngine->initialize(QString());

        QVERIFY(runLua(mpSecondHost, qsl("_sttSecondStarted = stt.start()")).isNull());
        QVERIFY2(luaGlobalBoolean(mpSecondHost, qsl("_sttSecondStarted")), "the second profile could not start a session");

        QVERIFY(runLua(mpFirstHost, qsl("_sttStopOk, _sttStopWhy = stt.stop()")).isNull());
        const bool stopSucceeded = luaGlobalBoolean(mpFirstHost, qsl("_sttStopOk"));
        const QString why = luaGlobalString(mpFirstHost, qsl("_sttStopWhy"));
        const bool stillListening = pEngine->listening();
        const Host* pOwner = mudlet::self()->microphoneOwner();

        runLua(mpSecondHost, qsl("stt.stop()"));
        retireStandInEngine();

        QVERIFY2(!stopSucceeded, "a profile holding no session was told it had stopped one");
        QVERIFY2(why.contains(qsl("only the profile that started a session can stop it")), qPrintable(qsl("the refusal does not say why: \"%1\"").arg(why)));
        QVERIFY2(stillListening, "another profile's stop ended the session anyway");
        QCOMPARE(pOwner, mpSecondHost);
    }

    // The microphone cannot change hands while the last phrase is still being
    // decoded: the result is owed to the profile that spoke it, and the claim
    // is what routes it there. Refused rather than waited for, since a decode
    // can outlive the call - the same answer a stop-then-start gets on a
    // backend that finalises asynchronously.
    void test_theMicrophoneIsNotTakenWhileAPhraseIsStillBeingDecoded()
    {
        mudlet::self()->activateProfile(mpSecondHost);
        StandInRecognizer* pEngine = installStandInEngine();
        QVERIFY2(pEngine, "the stand-in engine was not installed");
        pEngine->initialize(QString());

        QVERIFY(runLua(mpSecondHost, qsl("_sttDecodingStart = stt.start()")).isNull());
        QVERIFY2(luaGlobalBoolean(mpSecondHost, qsl("_sttDecodingStart")), "the second profile could not start a session");
        pEngine->beginProcessing();

        QVERIFY(runLua(mpFirstHost, qsl("_sttTakeOk, _sttTakeWhy = stt.start()")).isNull());
        const bool takeSucceeded = luaGlobalBoolean(mpFirstHost, qsl("_sttTakeOk"));
        const QString why = luaGlobalString(mpFirstHost, qsl("_sttTakeWhy"));
        const Host* pOwner = mudlet::self()->microphoneOwner();

        retireStandInEngine();

        QVERIFY2(!takeSucceeded, "the microphone was taken while the previous profile's phrase was still being decoded");
        QVERIFY2(why.contains(qsl("still finishing a phrase")), qPrintable(qsl("the refusal does not say why: \"%1\"").arg(why)));
        QCOMPARE(pOwner, mpSecondHost);
    }

    // A command created by a profile that is not the one on screen must arrive
    // hidden. Packages place their commands when they load, which is not
    // necessarily a moment their profile is the one being looked at.
    void test_aCommandCreatedByABackgroundProfileArrivesHidden()
    {
        mudlet::self()->activateProfile(mpFirstHost);

        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechLate\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);

        QAction* pEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechLate"));
        QVERIFY2(pEntry, "the command was not placed at all");
        QVERIFY2(!pEntry->isVisible(), "a command created by a profile that is not on screen was shown straight away");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The commonest arrangement of all, and the one the marker was built for:
    // two games as tabs in this window, the background one listening, the
    // player about to minimise Mudlet. Asking only about the shown tab left
    // that case with no marker anywhere.
    void test_theMainWindowMarksAListeningProfileOnABackgroundTab()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        const QString quiet = mudlet::self()->windowTitle();

        mudlet::self()->claimMicrophoneFor(mpSecondHost);
        QVERIFY2(mudlet::self()->windowTitle().contains(qsl("listening")), qPrintable(qsl("a profile listening on a background tab left the title unmarked: %1").arg(mudlet::self()->windowTitle())));

        mudlet::self()->releaseMicrophone();
        QCOMPARE(mudlet::self()->windowTitle(), quiet);
    }

    // A handover answers, and the answer is what the binding reports. The
    // refusal half of this rule - that a claim is turned down while the
    // outgoing profile's phrase is still being decoded, so its result is not
    // delivered to a game that never said it - needs a recognizer sitting in
    // Processing, and no engine exists in a test run. This pins the granted
    // path only; the refusal is covered by inspection rather than here.
    void test_aHandoverIsGrantedWhenNothingIsBeingDecoded()
    {
        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        QCOMPARE(mudlet::self()->microphoneOwner(), mpFirstHost);

        QVERIFY2(mudlet::self()->claimMicrophoneFor(mpSecondHost), "a handover was refused with nothing being decoded");
        QCOMPARE(mudlet::self()->microphoneOwner(), mpSecondHost);
    }

    // Per-window chrome must not outlive its window. The entry is keyed by the
    // window's address, and a detached window deletes itself when it closes.
    void test_chromeOfAClosedWindowIsForgotten()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechPruned\", menuPath = \"Pruned\""));
        QVERIFY(secondId > 0);

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);
        QVERIFY2(buttonIn(pWindow, qsl("SpeechPruned")), "the command did not move into the detached window");
        const int withWindowOpen = mudlet::self()->addonChromeWindowCount();
        QVERIFY2(withWindowOpen >= 2, "the detached window never took chrome of its own");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        mudlet::self()->refreshAddonPlacement();

        QVERIFY2(mudlet::self()->addonChromeWindowCount() < withWindowOpen, "the closed window's chrome is still recorded, keyed by an address that no longer belongs to it");
    }

    // Hiding a command's own item is not enough to take the submenu it sits
    // under off the menu: QMenu::isEmpty() counts visible actions, so "Speech"
    // stayed openable and opened onto nothing.
    void test_aSubmenuHoldingOnlyHiddenCommandsIsHiddenToo()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"Rec\", menuPath = \"SpeechMenu\", surfaces = \"menu\""));
        QVERIFY(secondId > 0);

        mudlet::self()->activateProfile(mpSecondHost);
        QAction* pSubmenu = menuItemIn(mudlet::self(), qsl("SpeechMenu"));
        QVERIFY2(pSubmenu, "the submenu was never created");
        QVERIFY2(pSubmenu->isVisible(), "the submenu is hidden while its own profile is being shown");

        mudlet::self()->activateProfile(mpFirstHost);
        QVERIFY2(!pSubmenu->isVisible(), "a submenu holding nothing but another profile's hidden commands is still on the menu");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // Focus leaving Mudlet - alt-tabbed to a browser, or into the script editor
    // - must not drag a pinned control back to its own profile's window. That
    // is the moment the player is furthest from it.
    void test_aPinnedCommandStaysPutWhenFocusLeavesEveryWindow()
    {
        const int firstId = addCommand(mpFirstHost, qsl("name = \"SpeechStay\", surfaces = \"toolbar\""));
        QVERIFY(firstId > 0);

        mudlet::self()->activateProfile(mpFirstHost);
        mudlet::self()->activateWindow();
        QVERIFY(runLua(mpFirstHost, qsl("setCommandPinned(%1, true)").arg(firstId)).isNull());

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);
        pWindow->activateWindow();
        QVERIFY2(QTest::qWaitFor(
                         [pWindow]() {
                             return QApplication::activeWindow() == pWindow;
                         },
                         2000),
                 "the detached window never became active, so this cannot test what happens when focus leaves it");
        QVERIFY2(buttonIn(pWindow, qsl("SpeechStay")), "the pinned command did not follow the player into the detached window");

        // Nothing of ours has focus now, which is what alt-tabbing away looks
        // like from in here. Through the slot the application's own
        // focusWindowChanged reaches, rather than by asking for a placement
        // pass directly: the line that decides where a pinned command goes is
        // in that slot, so a direct call leaves it out and the case passes
        // whether the rule holds or not.
        pWindow->activateWindow();
        QApplication::setActiveWindow(nullptr);
        QVERIFY(QMetaObject::invokeMethod(mudlet::self(), "slot_focusWindowChanged", Q_ARG(QWindow*, nullptr)));

        QVERIFY2(buttonIn(pWindow, qsl("SpeechStay")), "a pinned command was dragged back to its own profile's window when focus left Mudlet");

        QVERIFY(runLua(mpFirstHost, qsl("setCommandPinned(%1, false)").arg(firstId)).isNull());
        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(firstId));
    }

    // A moved command keeps its icon and its pulse, which the first version of
    // this rule recorded nowhere and would have lost on every drag-out.
    void test_aMovedCommandKeepsItsIconAndItsPulse()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechPaint\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        QVERIFY(runLua(mpSecondHost, qsl("setCommandIcon(%1, \"dialog-information\")").arg(secondId)).isNull());
        QVERIFY(runLua(mpSecondHost, qsl("setCommandPulse(%1, true, \"#ff4444\", \"#cc0000\", 500)").arg(secondId)).isNull());

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QToolButton* pMoved = buttonIn(pWindow, qsl("SpeechPaint"));
        QVERIFY2(pMoved, "the command did not arrive in the detached window");
        QVERIFY2(!pMoved->styleSheet().isEmpty(), "a pulsing command came out of the move unpainted");
        QVERIFY2(pMoved->styleSheet().contains(qsl("background-color")), "the pulse stylesheet did not survive the move");

        runLua(mpSecondHost, qsl("setCommandPulse(%1, false)").arg(secondId));
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The question this whole rule was written to answer: several games sharing
    // one window show one set of commands, not one set each. The main window
    // covers the same code path, but a detached window is where a player is
    // most likely to stack profiles up, and its own switchToProfile() is what
    // drives the swap there.
    void test_oneWindowWithTwoProfilesShowsOneSetOfCommands()
    {
        const int firstId = addCommand(mpFirstHost, qsl("name = \"SpeechOne\", surfaces = \"toolbar\""));
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechTwo\", surfaces = \"toolbar\""));
        QVERIFY(firstId > 0 && secondId > 0);

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        // Move the first profile in alongside the second, so one window holds
        // both - the arrangement four tabs in one detached window generalises
        QVERIFY2(pWindow->addProfile(mFirstHostname, mpFirstHost->mpConsole), "the first profile could not join the detached window");
        pWindow->switchToProfile(mFirstHostname);
        QTest::qWait(200ms);

        QCOMPARE(pWindow->getProfileCount(), 2);
        QCOMPARE(pWindow->getCurrentProfileName(), mFirstHostname);

        QAction* pOne = toolbarEntryIn(pWindow, qsl("SpeechOne"));
        QAction* pTwo = toolbarEntryIn(pWindow, qsl("SpeechTwo"));
        QVERIFY2(pOne, "the shown profile's command is not in the window at all");
        QVERIFY2(pOne->isVisible(), "the shown profile's command is not on the toolbar");
        QVERIFY2(!pTwo || !pTwo->isVisible(), "both profiles' commands are on one window's toolbar at once");

        pWindow->switchToProfile(mSecondHostname);
        QTest::qWait(200ms);
        QAction* pTwoNow = toolbarEntryIn(pWindow, qsl("SpeechTwo"));
        QVERIFY2(pTwoNow && pTwoNow->isVisible(), "switching tabs in the detached window did not bring the other profile's command out");
        QAction* pOneNow = toolbarEntryIn(pWindow, qsl("SpeechOne"));
        QVERIFY2(!pOneNow || !pOneNow->isVisible(), "switching tabs left the previous profile's command on the toolbar");

        pWindow->removeProfile(mFirstHostname);
        QTest::qWait(100ms);
        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(firstId));
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

private:
    TDetachedWindow* detachSecondProfile()
    {
        // Whichever profile sits at tab 1 detaches, and slot_tabDetachRequested
        // refuses index 0 - so this is the second profile by construction
        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));
        QTest::qWait(200ms);
        return mudlet::self()->getDetachedWindows().value(mSecondHostname);
    }
};

#include "SpeechAcrossProfilesTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpeechAcrossProfilesTest)
