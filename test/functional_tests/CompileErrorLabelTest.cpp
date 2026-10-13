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

/*
 * TLuaInterpreter::compile() calls a failure "Lua syntax error" only when the
 * code did not parse. Running out of memory while loading is not a syntax
 * error either, and no Lua code can make the allocator fail on cue, so this
 * swaps in one that does.
 *
 * Run with: ctest -R CompileErrorLabelTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

#include "GroupedTest.h"

namespace {
lua_Alloc sRealAlloc = nullptr;
void* sRealAllocData = nullptr;

// Lua requires a shrink or a free to succeed, so only growth is refused.
void* refuseGrowth(void*, void* ptr, size_t oldSize, size_t newSize)
{
    if (newSize > oldSize) {
        return nullptr;
    }
    return sRealAlloc(sRealAllocData, ptr, oldSize, newSize);
}
} // namespace

class CompileErrorLabelTest : public QObject
{
    Q_OBJECT

private:
    const QString mHostname = qsl("CompileErrorLabel-Test");
    const QString mLocalhost = qsl("127.0.0.1");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;

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
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "no active host available for the test");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
            TestProfile::removeProfileDirectory(mHostname);
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_runningOutOfMemoryWhileLoadingIsNotASyntaxError()
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        sRealAlloc = lua_getallocf(L, &sRealAllocData);
        lua_setallocf(L, refuseGrowth, nullptr);
        QString error;
        const bool compiled = mpHost->mLuaInterpreter.compile(qsl("compileErrorLabelTestValue = 1"), error, qsl("CompileErrorLabelTest"));
        lua_setallocf(L, sRealAlloc, sRealAllocData);

        QVERIFY(!compiled);
        QVERIFY2(error.contains(qsl("not enough memory")), qPrintable(error));
        QVERIFY2(!error.contains(qsl("syntax error")), qPrintable(qsl("a load that ran out of memory was reported as a syntax error: %1").arg(error)));
    }
};

#include "CompileErrorLabelTest.moc"
MUDLET_GROUPED_TEST_MAIN(CompileErrorLabelTest)
