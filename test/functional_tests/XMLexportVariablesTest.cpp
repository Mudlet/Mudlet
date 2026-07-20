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
 * Tests for XMLexport::writeVariablePackage(): variables created after the
 * VarUnit tree was last built (e.g. by scripts at runtime) must still be
 * written to the profile XML when they are marked as saved. The tree is
 * only (re)built at profile load and when the Variables view is populated,
 * so without a refresh at export time such variables silently vanish from
 * profile saves.
 *
 * Run with: ctest -R XMLexportVariablesTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "VarUnit.h"
#include "XMLexport.h"
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

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForXMLexportVariablesTest();

class XMLexportVariablesTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "XMLexportVars-Test";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForXMLexportVariablesTest();

        mpServer = new TelnetServerStub(qApp);
        // port 0 asks the OS for an ephemeral port, so parallel test runs
        // (and other worktrees) cannot collide on a fixed one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "TelnetServerStub failed to bind a loopback port");
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // A saved variable whose Lua value only comes into existence after the
    // variable tree was last built (profile load, Variables view opening)
    // must still be written out - the save path has to refresh the tree.
    void test_lateCreatedSavedVariableIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        // build the tree directly, standing in for the initial build that
        // profile load performs (via Host::hideMudletsVariables())
        lI->getVars(false);
        QVERIFY(vu->getBase());

        // a script creates the variable after that; we mark its name as saved
        // to emulate a variable persisted in a previous session (savedVars is
        // name-keyed and persistent, so it survives a tree rebuild)
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "lateSavedTestVar = 'created after tree build'"), 0);
        vu->savedVars.insert(qsl("lateSavedTestVar"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("lateSavedTestVar")),
                 "saved variable created after the last variable-tree build should "
                 "still be exported to the profile XML");
        // the value is the payload of the save - make sure it is written, not
        // just an empty node with the right name
        QVERIFY2(xml.contains(qsl("created after tree build")), "the saved variable's value must be exported, not just its name");

        // mpHost is shared across the tests, so undo the state this one added
        vu->savedVars.remove(qsl("lateSavedTestVar"));
        QCOMPARE(luaL_dostring(L, "lateSavedTestVar = nil"), 0);
    }

    // The export-time refresh must not start saving variables that are not
    // marked as saved.
    void test_lateUnsavedVariableIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        lI->getVars(false);

        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "lateUnsavedTestVar = 'not marked saved'"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(!xml.contains(qsl("lateUnsavedTestVar")), "a variable not marked as saved must not be exported");
    }

    // The export-time refresh must keep writing the user's hidden-variable
    // preferences to the HiddenVariables node.
    void test_hiddenPreferenceStillExported()
    {
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        vu->addHidden(qsl("userHiddenPrefVar"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("userHiddenPrefVar")), "hiddenByUser names must still be written to HiddenVariables");

        vu->removeHidden(qsl("userHiddenPrefVar"));
    }

private:
    QString exportProfileXml()
    {
        const QString xmlPath = mudlet::getMudletPath(enums::profileHomePath, mHostname) + qsl("/xmlexport-test.xml");
        auto writer = std::make_shared<XMLexport>(mpHost);
        if (!writer->exportPackage(xmlPath, true, false)) {
            return {};
        }
        QFile file(xmlPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        const QString xml = QString::fromUtf8(file.readAll());
        file.close();
        QFile::remove(xmlPath);
        return xml;
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(2000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(1000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResourcesForXMLexportVariablesTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "XMLexportVariablesTest.moc"
QTEST_MAIN(XMLexportVariablesTest)
