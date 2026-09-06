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
 * Run with: ctest -R SpeechAcrossProfilesTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
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

    Host* hostFor(const QString& profileName) const { return mudlet::self()->getHostManager().getHost(profileName); }

    // The connection dialog can only be driven once: with the main window up it
    // never takes activation again, so the second profile is opened the way a
    // player opens one from inside a running Mudlet instead.
    bool provisionProfileOnDisk(const QString& profileName) const
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, profileName)) && mudlet::self()->writeProfileData(profileName, qsl("url"), mLocalhost).first
               && mudlet::self()->writeProfileData(profileName, qsl("port"), mPort).first;
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

    void deleteProfileDirectory(const QString& profileName) const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

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

    // No test may leave the microphone claimed: the owner decides where the
    // next test's events land, so a leaked claim fails the one after it
    void cleanup() { mudlet::self()->releaseMicrophone(); }

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
        mudlet::self()->claimMicrophoneFor(mpFirstHost);

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
};

#include "SpeechAcrossProfilesTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpeechAcrossProfilesTest)
