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
 * Issue #9857. A saved table holding a function, userdata or coroutine cannot be
 * written out in full: the save drops those members and the next session gets a
 * table its own scripts no longer recognise - a cron job without its command,
 * say. Mudlet up to 4.22 wrote such a table as an empty group, and packages key
 * "if it is empty, rebuild it" off exactly that, so it goes on being written the
 * way it was: the members registered in savedVars and no others.
 *
 * The shapes below are the ones measured on 4.22.0 and on the 5.0 release
 * candidate, each ticked as a saved variable while empty and filled by a script
 * afterwards - which is the state a package that rebuilds its own tables is
 * always in, and the only one that regressed. Tables that hold nothing but data
 * keep the full save.
 *
 * Run with: ctest -R SavedVariableFenceTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "VarUnit.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QXmlStreamReader>

#include <optional>

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
void initializeQRCResourcesForSavedVariableFenceTest();

struct SavedShape
{
    // what the shape is called in the measurements this pins
    const char* description = nullptr;
    QString globalName;
    // Lua that builds the shape, standing in for the script that fills a table
    // the user ticked while it was empty
    QString setup;
    // the member names the save has to write inside the variable, so an empty
    // list means the variable is written as an empty group
    QStringList exportedMembers;
    // what the next session has to see
    QString restoreCheck;
};

class SavedVariableFenceTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "SavedVarFence-Test";
    const QString mLocalhost = "localhost";

    // An existing but empty table is what the "rebuild it if it is empty" guard
    // in cron-daemon and every package like it needs to see.
    static QString emptyTableCheck(const QString& globalName)
    {
        return qsl("assert(type(%1) == 'table', 'the saved variable did not come back as a table') "
                   "assert(next(%1) == nil, 'the saved variable came back partially filled')")
                .arg(globalName);
    }

    static QVector<SavedShape> shapes()
    {
        return {
                {"flat data", qsl("qaFlatData"), qsl("qaFlatData = {a = 1, b = 'x'}"), {qsl("a"), qsl("b")}, qsl("assert(qaFlatData.a == 1 and qaFlatData.b == 'x')")},
                {"clean nested",
                 qsl("qaCleanNested"),
                 qsl("qaCleanNested = {a = 1, sub = {b = 2, c = 'deep'}}"),
                 {qsl("a"), qsl("sub")},
                 qsl("assert(qaCleanNested.a == 1 and qaCleanNested.sub.b == 2 and qaCleanNested.sub.c == 'deep')")},
                {"two globals on one data table",
                 qsl("qaTwoGlobals"),
                 qsl("qaTwoGlobals = {a = 1, b = 'x'} qaTwoGlobalsAlias = qaTwoGlobals"),
                 {qsl("a"), qsl("b")},
                 qsl("assert(qaTwoGlobals.a == 1 and qaTwoGlobals.b == 'x')")},
                {"flat mixed", qsl("qaFlatMixed"), qsl("qaFlatMixed = {a = 1, f = function() end}"), {}, emptyTableCheck(qsl("qaFlatMixed"))},
                {"two globals on one table holding a function",
                 qsl("qaTwoGlobalsMixed"),
                 qsl("qaTwoGlobalsMixed = {a = 1, f = function() end} qaTwoGlobalsMixedAlias = qaTwoGlobalsMixed"),
                 {},
                 emptyTableCheck(qsl("qaTwoGlobalsMixed"))},
                {"cron-daemon's shape",
                 qsl("qaCron"),
                 qsl("qaCron = {otherdata = 1, joblist = {{command = function() end, spec = '*', name = 'j1'}, {command = function() end, spec = '*', name = 'j2'}}}"),
                 {},
                 emptyTableCheck(qsl("qaCron"))},
                {"function only", qsl("qaFunctionOnly"), qsl("qaFunctionOnly = {f = function() end}"), {}, emptyTableCheck(qsl("qaFunctionOnly"))},
                {"callbacks in a deeper table", qsl("qaDeepCallbacks"), qsl("qaDeepCallbacks = {data = {a = 1}, callbacks = {f = function() end}}"), {}, emptyTableCheck(qsl("qaDeepCallbacks"))},
                {"userdata", qsl("qaUserdata"), qsl("qaUserdata = {a = 1, ud = io.stdout}"), {}, emptyTableCheck(qsl("qaUserdata"))},
                {"coroutine", qsl("qaCoroutine"), qsl("qaCoroutine = {a = 1, co = coroutine.create(function() end)}"), {}, emptyTableCheck(qsl("qaCoroutine"))},
                {"array and hash with a function", qsl("qaArrayHash"), qsl("qaArrayHash = {1, 2, function() end, x = 1}"), {}, emptyTableCheck(qsl("qaArrayHash"))},
        };
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForSavedVariableFenceTest();

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

        // the userdata shape is built out of io.stdout, and a test that quietly
        // stopped covering userdata would still pass
        QCOMPARE(runLua(qsl("assert(type(io.stdout) == 'userdata')")), QString());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    void test_savedShapeExportsWhatItAlwaysDid_data()
    {
        QTest::addColumn<QString>("globalName");
        QTest::addColumn<QString>("setup");
        QTest::addColumn<QStringList>("exportedMembers");

        for (const SavedShape& shape : shapes()) {
            QTest::newRow(shape.description) << shape.globalName << shape.setup << shape.exportedMembers;
        }
    }

    // Each shape ticked while empty and filled by a script afterwards, so the
    // variable is registered in savedVars and none of its members are.
    void test_savedShapeExportsWhatItAlwaysDid()
    {
        QFETCH(QString, globalName);
        QFETCH(QString, setup);
        QFETCH(QStringList, exportedMembers);

        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(setup), QString());
        vu->savedVars.insert(globalName);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        const std::optional<QStringList> written = exportedMembersOf(xml, globalName);
        QVERIFY2(written.has_value(),
                 "the saved variable itself must be on disk whatever it holds - the next session seeing an empty table "
                 "rather than nil is what a package's rebuild guard keys off");

        QStringList expected = exportedMembers;
        expected.sort();
        QStringList got = written.value();
        got.sort();
        QCOMPARE(got, expected);

        vu->savedVars.remove(globalName);
        QCOMPARE(runLua(qsl("_G['%1'] = nil _G['%1Alias'] = nil").arg(globalName)), QString());
    }

    // The fallback is per saved variable, so one variable holding a function
    // cannot cost an unrelated one its full save.
    void test_aTableHoldingAFunctionDoesNotAffectItsSiblings()
    {
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(qsl("siblingDirtyTable = {a = 1, f = function() end} siblingCleanTable = {a = 'sibling clean value'}")), QString());
        vu->savedVars.insert(qsl("siblingDirtyTable"));
        vu->savedVars.insert(qsl("siblingCleanTable"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(exportedMembersOf(xml, qsl("siblingDirtyTable")).value_or(QStringList{qsl("missing")}), QStringList());
        QCOMPARE(exportedMembersOf(xml, qsl("siblingCleanTable")).value_or(QStringList{qsl("missing")}), QStringList{qsl("a")});

        vu->savedVars.remove(qsl("siblingDirtyTable"));
        vu->savedVars.remove(qsl("siblingCleanTable"));
        QCOMPARE(runLua(qsl("siblingDirtyTable, siblingCleanTable = nil")), QString());
    }

    // The upgrade path: a profile saved by 4.22 while the table was populated
    // carries a savedVars entry per member, and those keep being written whatever
    // else the table holds. Only the ride-along export of unregistered members
    // falls back.
    void test_registeredMembersOfATableHoldingAFunctionStillExport()
    {
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(qsl("registeredMixedTable = {a = 'registered member value', later = 'unregistered member value', f = function() end}")), QString());
        vu->savedVars.insert(qsl("registeredMixedTable"));
        vu->savedVars.insert(qsl("registeredMixedTable.a"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(exportedMembersOf(xml, qsl("registeredMixedTable")).value_or(QStringList{qsl("missing")}), QStringList{qsl("a")});
        QVERIFY2(xml.contains(qsl("registered member value")), "a member registered in savedVars must be exported whatever else its table holds");
        QVERIFY2(!xml.contains(qsl("unregistered member value")), "an unregistered member of a table holding a function must not be exported");

        vu->savedVars.remove(qsl("registeredMixedTable"));
        vu->savedVars.remove(qsl("registeredMixedTable.a"));
        QCOMPARE(runLua(qsl("registeredMixedTable = nil")), QString());
    }

    // What the user is left with next session. Declared last: the import puts a
    // whole profile package back into the live one, which the tests above would
    // then be sharing.
    void test_savedShapesRestoreAsTheyAlwaysDid()
    {
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        const QVector<SavedShape> savedShapes = shapes();
        // all of them in one profile, which is also how a save proves it treats
        // the variables one by one
        for (const SavedShape& shape : savedShapes) {
            QCOMPARE(runLua(shape.setup), QString());
            vu->savedVars.insert(shape.globalName);
        }

        const QString xmlPath = mudlet::getMudletPath(enums::profileHomePath, mHostname) + qsl("/saved-variable-fence-test.xml");
        QVERIFY2(exportProfileTo(xmlPath), "the profile could not be exported");

        for (const SavedShape& shape : savedShapes) {
            vu->savedVars.remove(shape.globalName);
            QCOMPARE(runLua(qsl("_G['%1'] = nil _G['%1Alias'] = nil").arg(shape.globalName)), QString());
        }

        QFile file(xmlPath);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), qPrintable(file.errorString()));
        XMLimport importer(mpHost);
        auto [imported, importError] = importer.importPackage(&file);
        file.close();
        QFile::remove(xmlPath);
        QVERIFY2(imported, qPrintable(importError));

        for (const SavedShape& shape : savedShapes) {
            const QString failure = runLua(shape.restoreCheck);
            QVERIFY2(failure.isEmpty(), qPrintable(qsl("%1: %2").arg(QLatin1String(shape.description), failure)));
        }

        for (const SavedShape& shape : savedShapes) {
            vu->savedVars.remove(shape.globalName);
            QCOMPARE(runLua(qsl("_G['%1'] = nil").arg(shape.globalName)), QString());
        }
    }

private:
    // Empty on success, the Lua error otherwise.
    QString runLua(const QString& code)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return {};
        }
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return error;
    }

    // Consumes the Variable or VariableGroup element the reader is on, handing
    // back the names of the member nodes written inside it.
    static QStringList readVariableNode(QXmlStreamReader& reader, QString& name)
    {
        QStringList members;
        while (reader.readNextStartElement()) {
            if (reader.name() == qsl("name")) {
                name = reader.readElementText();
            } else if (reader.name() == qsl("Variable") || reader.name() == qsl("VariableGroup")) {
                QString memberName;
                // the call consumes the member's own element, nested members and all
                readVariableNode(reader, memberName);
                members << memberName;
            } else {
                reader.skipCurrentElement();
            }
        }
        return members;
    }

    // The members the export wrote inside the variable called globalName, or
    // nothing at all when it wrote no such variable.
    static std::optional<QStringList> exportedMembersOf(const QString& xml, const QString& globalName)
    {
        QXmlStreamReader reader(xml);
        while (!reader.atEnd()) {
            if (reader.readNext() != QXmlStreamReader::StartElement || reader.name() != qsl("VariablePackage")) {
                continue;
            }
            while (reader.readNextStartElement()) {
                if (reader.name() != qsl("Variable") && reader.name() != qsl("VariableGroup")) {
                    reader.skipCurrentElement();
                    continue;
                }
                QString name;
                const QStringList members = readVariableNode(reader, name);
                if (name == globalName) {
                    return members;
                }
            }
        }
        return std::nullopt;
    }

    // The save runs on a worker thread, so the file is only there once its
    // future is.
    bool exportProfileTo(const QString& xmlPath)
    {
        auto writer = std::make_shared<XMLexport>(mpHost);
        if (!writer->exportPackage(xmlPath, true, false)) {
            return false;
        }
        for (QFuture<bool>& save : writer->saveFutures) {
            save.waitForFinished();
        }
        return true;
    }

    QString exportProfileXml()
    {
        const QString xmlPath = mudlet::getMudletPath(enums::profileHomePath, mHostname) + qsl("/saved-variable-fence-export.xml");
        if (!exportProfileTo(xmlPath)) {
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

void initializeQRCResourcesForSavedVariableFenceTest()
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

#include "SavedVariableFenceTest.moc"
QTEST_MAIN(SavedVariableFenceTest)
