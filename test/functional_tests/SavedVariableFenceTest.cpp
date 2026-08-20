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
 * written out in full: those members cannot be restored, and a table that comes
 * back with only some of its contents is one its own scripts no longer recognise
 * - a cron job without its command, say. Packages guard against that by
 * rebuilding a saved table when they find it empty, so a table registered while
 * empty has to come back empty rather than half-filled.
 *
 * Each shape below is registered as a saved variable and filled by a script
 * afterwards, so the variable has a savedVars entry and none of its members do,
 * which is the state a package that rebuilds its own tables is always in. Tables
 * that hold nothing but data keep the full save.
 *
 * Run with: ctest -R SavedVariableFenceTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
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
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

struct SavedShape
{
    // the QTest row name, and the prefix on a restore failure
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

struct ExportedVariable
{
    QString name;
    int valueType = 0;
    QStringList members;
};

class SavedVariableFenceTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "SavedVarFence-Test";
    const QString mLocalhost = "localhost";
    QSet<QString> mSavedVarsBefore;
    bool mHasImported = false;

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
                // one member of each type a save can carry, so a change to what
                // LuaInterface::setValue() rebuilds fails here
                {"flat data",
                 qsl("qaFlatData"),
                 qsl("qaFlatData = {a = 1, b = 'x', c = true, d = {e = 'nested'}}"),
                 {qsl("a"), qsl("b"), qsl("c"), qsl("d")},
                 qsl("assert(qaFlatData.a == 1 and qaFlatData.b == 'x' and qaFlatData.c == true and qaFlatData.d.e == 'nested')")},
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
                // a boundary rather than fence coverage: a table whose every
                // member is unsaveable exported empty before the fence too
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
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        // port 0 asks the OS for an ephemeral port, so parallel test runs
        // (and other worktrees) cannot collide on a fixed one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "TelnetServerStub failed to bind a loopback port");
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");

        // io.stdout is the only userdata to hand: if a Lua build ever stopped
        // providing it the userdata row would turn into a data-only table and
        // fail for a reason nobody would guess from the row name
        QCOMPARE(runLua(qsl("assert(type(io.stdout) == 'userdata')")), QString());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init() { mSavedVarsBefore = mpHost->getLuaInterface()->getVarUnit()->savedVars; }

    // QTest runs this after every test function and every data row, failed ones
    // included, so a row that fails part way cannot leave a global behind for the
    // next one to trip over. Every global these tests create is named qa*.
    void cleanup()
    {
        mpHost->getLuaInterface()->getVarUnit()->savedVars = mSavedVarsBefore;
        // setting an existing field to nil mid-traversal is the one mutation
        // Lua's next() allows
        QCOMPARE(runLua(qsl("for name in pairs(_G) do if type(name) == 'string' and name:find('^qa') then _G[name] = nil end end")), QString());
    }

    void test_savedShapeExportsItsExpectedMembers_data()
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
    void test_savedShapeExportsItsExpectedMembers()
    {
        QFETCH(QString, globalName);
        QFETCH(QString, setup);
        QFETCH(QStringList, exportedMembers);
        QVERIFY2(!mHasImported, "a test that imports a profile was declared before the ones that must run without it");

        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(setup), QString());
        vu->savedVars.insert(globalName);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        const std::optional<ExportedVariable> written = exportedVariable(xml, globalName);
        QVERIFY2(written.has_value(),
                 "the saved variable itself must be on disk whatever it holds - the next session seeing an empty table "
                 "rather than nil is what a package's rebuild guard keys off");
        // only a table node makes the import recreate the table, so an empty
        // group of any other type would restore as nil
        QCOMPARE(written->valueType, LUA_TTABLE);

        QStringList expected = exportedMembers;
        expected.sort();
        QStringList got = written->members;
        got.sort();
        QCOMPARE(got, expected);
    }

    // The fallback is per saved variable, so one variable holding a function
    // cannot cost an unrelated one its full save.
    void test_aTableHoldingAFunctionDoesNotAffectItsSiblings()
    {
        QVERIFY2(!mHasImported, "a test that imports a profile was declared before the ones that must run without it");
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(qsl("qaSiblingDirty = {a = 1, f = function() end} qaSiblingClean = {a = 'sibling clean value'}")), QString());
        vu->savedVars.insert(qsl("qaSiblingDirty"));
        vu->savedVars.insert(qsl("qaSiblingClean"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(exportedMemberNames(xml, qsl("qaSiblingDirty")), QStringList());
        QCOMPARE(exportedMemberNames(xml, qsl("qaSiblingClean")), QStringList{qsl("a")});
    }

    // Two saved globals that are the same table holding a function. Both have to
    // fence: the walk reads the table again under each saved name, and a fix that
    // read it once would leave whichever name Lua yielded second exporting empty
    // and the other exporting in full, by hash order.
    void test_bothSavedGlobalsSharingATableHoldingAFunctionFence()
    {
        QVERIFY2(!mHasImported, "a test that imports a profile was declared before the ones that must run without it");
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(qsl("qaSharedMixed = {a = 1, f = function() end} qaSharedMixedOther = qaSharedMixed")), QString());
        vu->savedVars.insert(qsl("qaSharedMixed"));
        vu->savedVars.insert(qsl("qaSharedMixedOther"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(exportedMemberNames(xml, qsl("qaSharedMixed")), QStringList());
        QCOMPARE(exportedMemberNames(xml, qsl("qaSharedMixedOther")), QStringList());
    }

    // A table that was already populated when the profile was saved comes back
    // with a savedVars entry per member - the import registers every element it
    // reads - and those keep being written whatever else the table holds. Only
    // the ride-along export of unregistered members falls back.
    void test_registeredMembersOfATableHoldingAFunctionStillExport()
    {
        QVERIFY2(!mHasImported, "a test that imports a profile was declared before the ones that must run without it");
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(qsl("qaRegisteredMixed = {a = 'registered member value', later = 'unregistered member value', f = function() end}")), QString());
        vu->savedVars.insert(qsl("qaRegisteredMixed"));
        vu->savedVars.insert(qsl("qaRegisteredMixed.a"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(exportedMemberNames(xml, qsl("qaRegisteredMixed")), QStringList{qsl("a")});
        QVERIFY2(xml.contains(qsl("registered member value")), "a member registered in savedVars must be exported whatever else its table holds");
        QVERIFY2(!xml.contains(qsl("unregistered member value")), "an unregistered member of a table holding a function must not be exported");
    }

    // Where the fence stops. A member the export drops for any reason other than
    // its value type still rides along, and its siblings with it: the 10,000-item
    // cap and the one-copy-per-saved-name rule are deliberate size limits, not
    // "this cannot be saved at all", and turning them into whole-variable
    // fallbacks would lose more data than they save. XMLexportVariablesTest pins
    // the drops themselves.
    void test_theFenceCoversValueTypesOnly()
    {
        QVERIFY2(!mHasImported, "a test that imports a profile was declared before the ones that must run without it");
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        QCOMPARE(runLua(qsl("qaReferenceKey = {a = 1} qaReferenceKey[{}] = 'reference member value' "
                            "qaSharedMember = {first = {b = 2}} qaSharedMember.second = qaSharedMember.first")),
                 QString());
        vu->savedVars.insert(qsl("qaReferenceKey"));
        vu->savedVars.insert(qsl("qaSharedMember"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(exportedMemberNames(xml, qsl("qaReferenceKey")), QStringList{qsl("a")});
        // whichever of the two names for that one table pairs() reaches first is
        // the one written, so only its count is deterministic
        QCOMPARE(exportedMemberNames(xml, qsl("qaSharedMember")).size(), 1);
    }

    // What the user is left with next session. Declared last: the import puts a
    // whole profile package back into the live one, which the tests above would
    // then be sharing.
    void test_savedShapesRestoreWholeOrEmpty()
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

        // nothing of the seeding may survive into the checks below, or they would
        // be reading what this session left in _G rather than what came back
        for (const SavedShape& shape : savedShapes) {
            vu->savedVars.remove(shape.globalName);
            QCOMPARE(runLua(qsl("_G['%1'] = nil _G['%1Alias'] = nil").arg(shape.globalName)), QString());
        }

        QFile file(xmlPath);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), qPrintable(file.errorString()));
        XMLimport importer(mpHost);
        mHasImported = true;
        auto [imported, importError] = importer.importPackage(&file);
        file.close();
        QFile::remove(xmlPath);
        QVERIFY2(imported, qPrintable(importError));

        for (const SavedShape& shape : savedShapes) {
            const QString failure = runLua(shape.restoreCheck);
            QVERIFY2(failure.isEmpty(), qPrintable(qsl("%1: %2").arg(QLatin1String(shape.description), failure)));
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
    // back what was written inside it.
    static ExportedVariable readVariableNode(QXmlStreamReader& reader)
    {
        ExportedVariable written;
        while (reader.readNextStartElement()) {
            if (reader.name() == qsl("name")) {
                written.name = reader.readElementText();
            } else if (reader.name() == qsl("valueType")) {
                written.valueType = reader.readElementText().toInt();
            } else if (reader.name() == qsl("Variable") || reader.name() == qsl("VariableGroup")) {
                // the call consumes the member's own element, nested members and all
                written.members << readVariableNode(reader).name;
            } else {
                reader.skipCurrentElement();
            }
        }
        return written;
    }

    // What the export wrote for the variable called globalName; std::nullopt when
    // it wrote no such variable at all, which an empty member list does not mean
    // - that is a variable written as an empty group.
    static std::optional<ExportedVariable> exportedVariable(const QString& xml, const QString& globalName)
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
                const ExportedVariable written = readVariableNode(reader);
                if (written.name == globalName) {
                    return written;
                }
            }
        }
        return std::nullopt;
    }

    // A variable the export did not write at all comes back naming that, so a
    // QCOMPARE against the expected members reports it instead of passing.
    static QStringList exportedMemberNames(const QString& xml, const QString& globalName)
    {
        const std::optional<ExportedVariable> written = exportedVariable(xml, globalName);
        return written ? written->members : QStringList{qsl("<the export wrote no such variable>")};
    }

    bool exportProfileTo(const QString& xmlPath)
    {
        auto writer = std::make_shared<XMLexport>(mpHost);
        return writer->exportPackage(xmlPath, true, false);
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
        auto host = TestProfile::create(hostname, address, port);
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

#include "SavedVariableFenceTest.moc"
MUDLET_GROUPED_TEST_MAIN(SavedVariableFenceTest)
