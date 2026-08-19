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
 * saveWindowLayout() from Lua must not stand in for the layout save Mudlet
 * makes on the way out. That save is guarded by mudlet::mHasSavedLayout, which
 * only a later layout change lowers again, and the flag is not readable from
 * Lua at all.
 *
 * Run with: ctest -R WindowLayoutSaveTest -V
 */

#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
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

class WindowLayoutSaveTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("WindowLayoutSave-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    // the layout file is shared by every profile and lives beside them rather
    // than inside one, so this test gets a configuration directory of its own
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;

    static bool portableMarkerPresent() { return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())); }

    // Returns the Lua error, or a null QString when the chunk ran
    QString runLua(const QString& code) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    bool luaGlobalIsTrue(const QString& name) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const bool value = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return value;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        QVERIFY(mConfigDir.isValid());
        // setupConfig() only adopts $XDG_CONFIG_HOME once the profiles
        // directory under it is there to be adopted
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // a settings file that already holds something is how mudletUsedBefore()
        // recognises an existing player, which keeps the first-run tour and the
        // starter UI package away from this profile
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();
        delete mudlet::self();
        if (mSavedXdgConfigHome.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        }
    }

    void test_aScriptedSaveLeavesTheShutdownSaveToRun()
    {
        mudlet::self()->mHasSavedLayout = false;

        QVERIFY(runLua(qsl("_layoutSaved = saveWindowLayout()")).isNull());
        QVERIFY2(luaGlobalIsTrue(qsl("_layoutSaved")), "saveWindowLayout() did not report a save");
        QVERIFY2(!mudlet::self()->mHasSavedLayout, "a scripted saveWindowLayout() raised the flag that turns the save at shutdown into a no-op");

        // which is the save TMainConsole::closeEvent() asks for on the way out
        QVERIFY2(mudlet::self()->saveWindowLayout(), "the layout save Mudlet makes when it closes was refused");
    }

    void test_aScriptedSaveIsNotRefusedAfterAnAutomaticOne()
    {
        mudlet::self()->mHasSavedLayout = false;
        QVERIFY(mudlet::self()->saveWindowLayout());
        QVERIFY2(mudlet::self()->mHasSavedLayout, "a save through Mudlet's own path did not record itself");

        QVERIFY(runLua(qsl("_layoutSaved = saveWindowLayout()")).isNull());
        QVERIFY2(luaGlobalIsTrue(qsl("_layoutSaved")), "saveWindowLayout() is refused once Mudlet has saved the layout itself");
        QVERIFY2(mudlet::self()->mHasSavedLayout, "a scripted saveWindowLayout() lowered a flag it did not raise");
    }
};

#include "WindowLayoutSaveTest.moc"
MUDLET_GROUPED_TEST_MAIN(WindowLayoutSaveTest)
