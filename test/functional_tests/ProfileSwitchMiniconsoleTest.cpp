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
 * A miniconsole works out how many columns it has from the width of its pane,
 * and a profile that is not the front tab is not laid out, so a miniconsole in
 * one can come back from a switch holding a column count worked out against a
 * width it no longer has - which is #8273, text cut off down the right hand
 * side. mudlet::setActiveProfile() answers that with a deferred
 * TMainConsole::refreshSubconsoles(), and nothing covered it.
 *
 * What this holds: after a switch, a miniconsole in either profile has columns
 * and rows, and a refresh right now would not change them. The check is not a
 * count this test knows in advance - that would only be the formula written out
 * twice - but whether the switch left anything for a refresh to fix.
 *
 * What this does NOT hold, measured rather than assumed: taking the deferred
 * refreshSubconsoles() call back out of mudlet::setActiveProfile() does not
 * make this fail - five runs each way under bare Xvfb, and five each way under
 * Xvfb with openbox laying out asynchronously, all passed. A miniconsole is
 * resized along with the console it sits in even while its profile is behind
 * another, so its own resize event has recalculated everything the refresh
 * would have by the time anything can look. So this pins the invariant against
 * a future regression; it is not evidence that the deferred refresh is
 * load-bearing, and #8273 itself is not reproduced here.
 *
 * Run with: ctest -R ProfileSwitchMiniconsoleTest -V
 */

#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TTextEdit.h"
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

class ProfileSwitchMiniconsoleTest : public QObject
{
    Q_OBJECT

private:
    static constexpr int csmConnectBudgetMs = 2000;

    TelnetServerStub* mpServer = nullptr;
    Host* mpFirstHost = nullptr;
    Host* mpSecondHost = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstProfile = qsl("ProfileSwitchMiniconsole-First");
    const QString mSecondProfile = qsl("ProfileSwitchMiniconsole-Second");
    const QString mMiniconsoleName = qsl("switchSpecMiniconsole");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    Host* hostFor(const QString& profileName) const { return mudlet::self()->getHostManager().getHost(profileName); }

    bool provisionProfileOnDisk(const QString& profileName) const
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, profileName)) && mudlet::self()->writeProfileData(profileName, qsl("url"), mLocalhost).first
               && mudlet::self()->writeProfileData(profileName, qsl("port"), mPort).first;
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

    int luaGlobalNumber(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // what a script sees of the miniconsole, which is the half of #8273 a
    // player would notice
    bool readGeyserSize(Host* pHost, int& width, int& height) const
    {
        const QString error = runLua(pHost, qsl("_switchWidth, _switchHeight = _switchMiniconsole:get_width(), _switchMiniconsole:get_height()"));
        if (!error.isNull()) {
            qWarning() << "readGeyserSize:" << error;
            return false;
        }
        width = luaGlobalNumber(pHost, qsl("_switchWidth"));
        height = luaGlobalNumber(pHost, qsl("_switchHeight"));
        return true;
    }

    TConsole* miniconsoleOf(Host* pHost) const { return pHost->mpConsole ? pHost->mpConsole->mSubConsoleMap.value(mMiniconsoleName) : nullptr; }

    bool createMiniconsole(Host* pHost) const
    {
        const QString error = runLua(pHost,
                                     qsl("_switchMiniconsole = Geyser.MiniConsole:new({\n"
                                         "  name = '%1', x = 0, y = 0, width = '50%', height = '50%'\n"
                                         "})\n"
                                         "_switchMiniconsole:echo('a line of text to lose the right hand side of')")
                                             .arg(mMiniconsoleName));
        if (!error.isNull()) {
            qWarning() << "createMiniconsole:" << error;
            return false;
        }
        return true;
    }

    // the tab bar is what setActiveProfile() drives, and what a player clicks
    void switchTo(Host* pHost)
    {
        mudlet::self()->mpTabBar->setCurrentIndex(mudlet::self()->mpTabBar->tabIndex(pHost->getName()));
        // the refresh the switch asks for runs from a zero-timer, so it has not
        // happened yet when setActiveProfile() returns
        QTest::qWait(200ms);
        QCoreApplication::processEvents();
    }

    // Whether a refresh right now would change what the miniconsole thinks it
    // holds. Reading it back after asking for one is the same question the
    // switch had to answer, without this test working the answer out itself.
    void verifyNothingLeftToRefresh(Host* pHost, const char* what)
    {
        TConsole* pMiniconsole = miniconsoleOf(pHost);
        QVERIFY2(pMiniconsole, what);
        QVERIFY(pMiniconsole->mUpperPane);

        const int reportedColumns = pMiniconsole->mUpperPane->getColumnCount();
        const int reportedRows = pMiniconsole->mUpperPane->getRowCount();
        QVERIFY2(reportedColumns > 0, qPrintable(qsl("%1 came back from the switch holding no columns at all").arg(QString::fromUtf8(what))));

        pMiniconsole->refreshView();
        const int refreshedColumns = pMiniconsole->mUpperPane->getColumnCount();
        const int refreshedRows = pMiniconsole->mUpperPane->getRowCount();

        QVERIFY2(reportedColumns == refreshedColumns && reportedRows == refreshedRows,
                 qPrintable(qsl("%1 was left holding %2x%3 by the switch, and a refresh makes that %4x%5")
                                    .arg(QString::fromUtf8(what), QString::number(reportedColumns), QString::number(reportedRows), QString::number(refreshedColumns), QString::number(refreshedRows))));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        QVERIFY(mConfigDir.isValid());
        // $XDG_CONFIG_HOME/mudlet/profiles is the opt-in that makes setupConfig()
        // adopt it, so this never opens a profile beside the developer's own
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
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString profileName = mFirstProfile;
        const QString address = mLocalhost;
        const QString port = mPort;
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), profileName);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        QVERIFY2(spy.wait(csmConnectBudgetMs), "the first profile took too long to load");
        mpFirstHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpFirstHost, "no active host after creating the first profile");
        QVERIFY2(mpFirstHost->mFORCE_SAVE_ON_EXIT, "profiles must save without asking, or a close puts up a modal question");

        QVERIFY(provisionProfileOnDisk(mSecondProfile));
        QVERIFY2(runLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(mSecondProfile)).isNull(), "the second profile could not be loaded");
        QTest::qWait(500ms);
        mpSecondHost = hostFor(mSecondProfile);
        QVERIFY2(mpSecondHost, "the second profile did not open");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpFirstHost = nullptr;
        mpSecondHost = nullptr;
        delete mudlet::self();
        if (mSavedXdgConfigHome.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        }
    }

    void test_aMiniconsoleComesBackFromAProfileSwitchLaidOut()
    {
        QVERIFY(createMiniconsole(mpFirstHost));
        QVERIFY(createMiniconsole(mpSecondHost));
        QTest::qWait(200ms);

        int firstWidth = 0;
        int firstHeight = 0;
        QVERIFY(readGeyserSize(mpFirstHost, firstWidth, firstHeight));
        QVERIFY2(firstWidth > 0 && firstHeight > 0, "the miniconsole was created with no size to speak of");

        // The window is resized while the second profile is behind the first,
        // so the console it comes back to is not the one it was laid out for -
        // which is the way a profile switch leaves a miniconsole stale.
        for (int round = 1; round <= 3; ++round) {
            switchTo(mpFirstHost);
            mudlet::self()->resize(1000 + round * 120, 700 + round * 40);
            QTest::qWait(200ms);

            switchTo(mpSecondHost);
            verifyNothingLeftToRefresh(mpSecondHost, "the second profile's miniconsole");

            mudlet::self()->resize(1200 - round * 100, 720 - round * 30);
            QTest::qWait(200ms);

            switchTo(mpFirstHost);
            verifyNothingLeftToRefresh(mpFirstHost, "the first profile's miniconsole");
        }

        // and a script asking the front profile's miniconsole how big it is
        // gets an answer that matches the window it is now in
        int widthAfter = 0;
        int heightAfter = 0;
        QVERIFY(readGeyserSize(mpFirstHost, widthAfter, heightAfter));
        QVERIFY2(widthAfter > 0 && heightAfter > 0, qPrintable(qsl("the miniconsole reports %1x%2 to Lua after the switching").arg(QString::number(widthAfter), QString::number(heightAfter))));
    }
};

#include "ProfileSwitchMiniconsoleTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileSwitchMiniconsoleTest)
