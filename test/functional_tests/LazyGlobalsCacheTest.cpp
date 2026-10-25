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

/*
 * Once Mudlet's handlers are seen on the globals metatable with no script
 * running, a line from the game does not look for them again until a script
 * has run. A package holding that metatable past the guard can take a handler
 * off from any way into Lua, so each one is tried here between two lines that
 * arrive with no script on the stack. A spec cannot reach this: every line a
 * spec feeds is processed while the spec itself is running.
 *
 * Run with: ctest -R LazyGlobalsCacheTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TEvent.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

#include "GroupedTest.h"

class LazyGlobalsCacheTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QByteArray mSavedTestMode;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "LazyGlobalsCache-Test";
    const QString mLocalhost = "localhost";

    void feed(const QString& text)
    {
        QByteArray data = text.toUtf8();
        mpHost->mTelnet.loopbackTest(data);
    }

    void run(const QString& code) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(code), qPrintable(code)); }

    QString rawGlobalString(const char* name)
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_pushstring(L, name);
        lua_rawget(L, LUA_GLOBALSINDEX);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : qsl("<%1>").arg(lua_typename(L, lua_type(L, -1)));
        lua_pop(L, 1);
        return value;
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
        // for rearmLazyGlobals(), which stands in for the package below
        mSavedTestMode = qgetenv("MUDLET_TEST_MODE");
        qputenv("MUDLET_TEST_MODE", "1");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host after profile creation");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "Could not connect with the host.");

        // The generic mapper reads "line" on every line, which runs a script
        // between any two lines and would hide what is tested here
        run(qsl("disableTrigger('onNewLine Trigger')"));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedTestMode.isNull() ? qunsetenv("MUDLET_TEST_MODE") : qputenv("MUDLET_TEST_MODE", mSavedTestMode);
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void handlerTakenOffBetweenLines_data()
    {
        QTest::addColumn<QString>("entry");
        QTest::newRow("script") << qsl("script");
        QTest::newRow("trigger") << qsl("trigger");
        QTest::newRow("alias") << qsl("alias");
        QTest::newRow("timer") << qsl("timer");
        QTest::newRow("event") << qsl("event");
    }

    void handlerTakenOffBetweenLines()
    {
        QFETCH(QString, entry);
        const QString strip = qsl("function() LazyCacheCopy.__index = nil LazyCacheStripped = true end");

        // Holding the globals metatable as such a package would: the guard sees
        // it handed out, and rearmLazyGlobals() turns the deferral back on
        run(qsl("LazyCacheStripped = nil\n"
                "LazyCacheOriginal = getmetatable(_G)\n"
                "LazyCacheCopy = {__index = LazyCacheOriginal.__index, __newindex = LazyCacheOriginal.__newindex}\n"
                "setmetatable(_G, LazyCacheCopy)\n"
                "assert(rearmLazyGlobals(), 'the deferral did not come back on')\n"
                "LazyCacheIds = {\n"
                "  target = tempRegexTrigger('^LazyCacheTarget (\\\\w+)$', function()\n"
                "    LazyCacheLine = line\n"
                "    LazyCacheCapture = matches and matches[2]\n"
                "  end),\n"
                "  trigger = tempRegexTrigger('^LazyCacheStrip$', %1),\n"
                "  alias = tempAlias('^lazycachestrip$', %1),\n"
                "  event = registerAnonymousEventHandler('LazyCacheStripEvent', %1),\n"
                "}")
                    .arg(strip));
        if (entry == qsl("timer")) {
            run(qsl("tempTimer(0, %1)").arg(strip));
        }

        feed(qsl("LazyCacheIdle\n"));
        QVERIFY2(mpHost->getLuaInterpreter()->mGlobalsHandlersVerified, "a script ran on the idle line, so nothing here exercises the remembered check");
        QCOMPARE(rawGlobalString("line"), qsl("<nil>"));

        if (entry == qsl("script")) {
            run(qsl("(%1)()").arg(strip));
        } else if (entry == qsl("trigger")) {
            feed(qsl("LazyCacheStrip\n"));
        } else if (entry == qsl("alias")) {
            mpHost->send(qsl("lazycachestrip"), false);
        } else if (entry == qsl("timer")) {
            QVERIFY(QTest::qWaitFor(
                    [this]() {
                        return rawGlobalString("LazyCacheStripped") == qsl("<boolean>");
                    },
                    2s));
        } else if (entry == qsl("event")) {
            TEvent event{};
            event.mArgumentList.append(qsl("LazyCacheStripEvent"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mpHost->raiseEvent(event);
        }
        // Read raw, as running a script to ask would itself forget the check
        QCOMPARE(rawGlobalString("LazyCacheStripped"), qsl("<boolean>"));

        feed(qsl("LazyCacheTarget word\n"));
        const QString seenLine = rawGlobalString("LazyCacheLine");
        const QString seenCapture = rawGlobalString("LazyCacheCapture");

        run(qsl("setmetatable(_G, LazyCacheOriginal)\n"
                "killTrigger(LazyCacheIds.target) killTrigger(LazyCacheIds.trigger) killAlias(LazyCacheIds.alias)\n"
                "killAnonymousEventHandler(LazyCacheIds.event)\n"
                "LazyCacheLine, LazyCacheCapture = nil, nil"));

        QCOMPARE(seenLine, qsl("LazyCacheTarget word"));
        QCOMPARE(seenCapture, qsl("word"));
    }
};

#include "LazyGlobalsCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(LazyGlobalsCacheTest)
