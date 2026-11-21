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
 * The copy of mudlet-lua an app that embeds Mudlet builds into the library (USE_BUILTIN_MUDLET_LUA), which
 * is loaded ahead of any copy on disk. The copy is registered here from a file, so that only this test
 * loads it. Each test makes a session of its own, and with it the process's one profile pool.
 */

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLockFile>
#include <QResource>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <chrono>
#include <filesystem>
#include <thread>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

class BuiltinMudletLuaTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QTemporaryDir mCacheDir;
    QByteArray mSavedXdgCache;
    const QString mBuiltinPath = qsl(":/builtin/mudlet-lua/lua");

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)");
        return true;
    }

    static bool writeFile(const QString& pathFileName, const QByteArray& content = QByteArray())
    {
        QFile file(pathFileName);
        return QDir().mkpath(QFileInfo(pathFileName).absolutePath()) && file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
    }

    // What tostring() makes of the Lua expression
    static QString luaValue(THeadlessSession& session, const QString& expression)
    {
        if (!session.runLua(qsl("builtinLuaTestValue = tostring(%1)").arg(expression))) {
            return qsl("<%1 failed>").arg(expression);
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "builtinLuaTestValue");
        const QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
    }

    static QString cacheLocation() { return QStandardPaths::writableLocation(QStandardPaths::CacheLocation); }

    static bool setTwoHoursOld(const QString& pathName)
    {
        std::error_code error;
        std::filesystem::last_write_time(pathName.toStdString(), std::filesystem::file_time_type::clock::now() - std::chrono::hours(2), error);
        return !error;
    }

    // Named after every file's name and content, so that a new version's copy gets a folder of its own
    QString unpackedFolder() const
    {
        QStringList files;
        QDirIterator it(mBuiltinPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            files << it.next().mid(mBuiltinPath.size() + 1);
        }
        files.sort();
        QCryptographicHash hash(QCryptographicHash::Sha1);
        for (const QString& name : std::as_const(files)) {
            QFile file(qsl("%1/%2").arg(mBuiltinPath, name));
            if (!file.open(QIODevice::ReadOnly)) {
                return qsl("<%1 cannot be read>").arg(name);
            }
            const QByteArray content = file.readAll();
            hash.addData(name.toUtf8() + '\0' + QByteArray::number(content.size()) + '\0');
            hash.addData(content);
        }
        return qsl("%1/mudlet-lua-%2").arg(cacheLocation(), QLatin1String(hash.result().toHex()));
    }

#if defined(INCLUDE_BUILTIN_MUDLET_LUA)
    static bool registerBuiltinCopy() { return true; }
    static void unregisterBuiltinCopy() {}
#else
    static bool registerBuiltinCopy() { return QResource::registerResource(qsl(BUILTIN_MUDLET_LUA_RCC)); }
    static void unregisterBuiltinCopy() { QResource::unregisterResource(qsl(BUILTIN_MUDLET_LUA_RCC)); }
#endif

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mCacheDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
        qputenv("XDG_CACHE_HOME", mCacheDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // Qt takes its cache folder from the environment everywhere but on macOS and Windows
        if (!cacheLocation().startsWith(mCacheDir.path())) {
#if defined(Q_OS_MACOS) || defined(Q_OS_WINDOWS)
            QSKIP("The cache folder cannot be redirected here, and these tests empty it");
#else
            QFAIL("XDG_CACHE_HOME did not move the cache folder, which these tests empty");
#endif
        }
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
    }

    void cleanup()
    {
        QFile::remove(cacheLocation());
        QDir(cacheLocation()).removeRecursively();
    }

    void test_theBuiltInCopyIsLoadedFromAFolderItIsUnpackedTo()
    {
        const QString name = qsl("Builtin Lua Unpacked");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        const QString folder = unpackedFolder();

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QCOMPARE(luaValue(session, qsl("luaGlobalPath")), folder);
        // What LuaGlobal.lua loads, a file in a folder of its own among them
        QCOMPARE(luaValue(session, qsl("debug.getinfo(string.cut, 'S').source")), qsl("@%1/StringUtils.lua").arg(folder));
        QCOMPARE(luaValue(session, qsl("debug.getinfo(Geyser.Label.new, 'S').source")), qsl("@%1/geyser/GeyserLabel.lua").arg(folder));
        // Lua's own file functions reach it, as scripts and loadTranslations() expect of luaGlobalPath
        QCOMPARE(luaValue(session, qsl("lfs.attributes(luaGlobalPath .. '/LuaGlobal.lua', 'mode')")), qsl("file"));
        // Where loadTranslations() looks once it has found no translations beside the program
        QCOMPARE(luaValue(session, qsl("type(loadTranslations('AdjustableContainer', 'mudlet-lua', 'de_DE', luaGlobalPath .. '/translations/'))")), qsl("table"));
        QCOMPARE(luaValue(session, qsl("lfs.attributes(luaGlobalPath .. '/translations/translated/mudlet-lua_de_DE.json', 'mode')")), qsl("file"));
    }

    void test_withNoBuiltInCopyTheOneOnDiskIsLoaded()
    {
#if defined(INCLUDE_BUILTIN_MUDLET_LUA)
        QSKIP("This build always has the built-in copy.");
#endif
        const QString name = qsl("Builtin Lua Absent");
        QVERIFY(writeProfile(name));

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QCOMPARE(luaValue(session, qsl("type(Geyser)")), qsl("table"));
        const QString luaGlobalPath = luaValue(session, qsl("luaGlobalPath"));
        QVERIFY2(!luaGlobalPath.startsWith(cacheLocation()) && !luaGlobalPath.startsWith(QLatin1Char(':')), qPrintable(luaGlobalPath));
        QVERIFY(QDir(cacheLocation()).entryList({qsl("*mudlet-lua-*")}, QDir::Dirs).isEmpty());
    }

    void test_aCopyThatCannotBeUnpackedIsLoadedFromTheLibrary()
    {
        const QString name = qsl("Builtin Lua Not Unpacked");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        // A file where the cache folder goes
        QVERIFY(QDir(cacheLocation()).removeRecursively());
        QVERIFY(writeFile(cacheLocation()));

        THeadlessSession session;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("could not unpack the built-in Lua files")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QCOMPARE(luaValue(session, qsl("luaGlobalPath")), mBuiltinPath);
        QCOMPARE(luaValue(session, qsl("debug.getinfo(string.cut, 'S').source")), qsl("@%1/StringUtils.lua").arg(mBuiltinPath));
        QCOMPARE(luaValue(session, qsl("debug.getinfo(Geyser.Label.new, 'S').source")), qsl("@%1/geyser/GeyserLabel.lua").arg(mBuiltinPath));
    }

    // With the copy still to unpack, and with it there already
    void test_otherVersionsUnpackedCopiesAreRemovedOnceOutOfUse()
    {
        const QString name = qsl("Builtin Lua Other Versions");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        const QString unusedVersion = qsl("%1/mudlet-lua-%2").arg(cacheLocation(), QString(40, QLatin1Char('0')));
        // One another Mudlet may be loading, though it was unpacked long ago
        const QString versionInUse = qsl("%1/mudlet-lua-%2").arg(cacheLocation(), QString(40, QLatin1Char('1')));
        const QString notACopy = qsl("%1/mudlet-lua-%2x").arg(cacheLocation(), QString(40, QLatin1Char('0')));
        // What a Mudlet stopped while it unpacked leaves, and one that may still be unpacking
        const QString abandoned = qsl("%1/unpacking-mudlet-lua-abcdef").arg(cacheLocation());
        const QString underWay = qsl("%1/unpacking-mudlet-lua-ghijkl").arg(cacheLocation());
        const QString unusedLastUse = qsl("%1/last-use").arg(unusedVersion);
        const QString ownLastUse = qsl("%1/last-use").arg(unpackedFolder());

        for (const bool unpackedAlready : {false, true}) {
            QCOMPARE(QFileInfo::exists(unpackedFolder()), unpackedAlready);
            QVERIFY(writeFile(qsl("%1/geyser/Geyser.lua").arg(unusedVersion)));
            QVERIFY(writeFile(unusedLastUse));
            QVERIFY(setTwoHoursOld(unusedLastUse));
            QVERIFY(writeFile(qsl("%1/geyser/Geyser.lua").arg(versionInUse)));
            QVERIFY(writeFile(qsl("%1/last-use").arg(versionInUse)));
            QVERIFY(setTwoHoursOld(versionInUse));
            QVERIFY(writeFile(qsl("%1/notes.txt").arg(notACopy)));
            QVERIFY(setTwoHoursOld(notACopy));
            QVERIFY(writeFile(qsl("%1/LuaGlobal.lua").arg(abandoned)));
            QVERIFY(setTwoHoursOld(abandoned));
            QVERIFY(writeFile(qsl("%1/LuaGlobal.lua").arg(underWay)));
            if (unpackedAlready) {
                QVERIFY(setTwoHoursOld(ownLastUse));
            }

            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

            QCOMPARE(luaValue(session, qsl("luaGlobalPath")), unpackedFolder());
            QVERIFY(!QFileInfo::exists(unusedVersion));
            QVERIFY(QFileInfo::exists(versionInUse));
            QVERIFY(QFileInfo::exists(qsl("%1/notes.txt").arg(notACopy)));
            QVERIFY(!QFileInfo::exists(abandoned));
            QVERIFY(QFileInfo::exists(underWay));
            // Which is what keeps another version's Mudlet from removing this copy
            QVERIFY(QFileInfo::exists(ownLastUse));
            QVERIFY(QFileInfo(ownLastUse).lastModified().secsTo(QDateTime::currentDateTime()) < 600);
        }
    }

    // A profile left open longer than another version waits before removing a copy still reads from it
    void test_aCopyInUseIsMarkedSoWhileThisMudletRuns()
    {
        const QString name = qsl("Builtin Lua Kept In Use");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        const QString ownLastUse = qsl("%1/last-use").arg(unpackedFolder());

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(luaValue(session, qsl("luaGlobalPath")), unpackedFolder());

        auto* refresh = qApp->findChild<QTimer*>(qsl("builtinLuaInUseRefresh"));
        QVERIFY(refresh);
        QVERIFY(refresh->isActive());
        QVERIFY(refresh->intervalAsDuration() <= std::chrono::minutes(30));
        QVERIFY(setTwoHoursOld(ownLastUse));
        const auto interval = refresh->intervalAsDuration();
        const auto restore = qScopeGuard([refresh, interval] {
            refresh->start(interval);
        });
        refresh->start(std::chrono::milliseconds(10));
        QTRY_VERIFY(QFileInfo(ownLastUse).lastModified().secsTo(QDateTime::currentDateTime()) < 600);
    }

    void test_theCopyIsUnpackedAllTheSameWhileAnotherMudletHoldsTheLock()
    {
        const QString name = qsl("Builtin Lua Locked");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        QVERIFY(QDir().mkpath(cacheLocation()));
        QLockFile heldElsewhere(qsl("%1/mudlet-lua.lock").arg(cacheLocation()));
        QVERIFY(heldElsewhere.tryLock());

        THeadlessSession session;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("another Mudlet still holds .*mudlet-lua.lock")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QCOMPARE(luaValue(session, qsl("luaGlobalPath")), unpackedFolder());
    }

    void test_theCopyIsUnpackedOnceAnotherMudletLetsGoOfTheLock()
    {
        const QString name = qsl("Builtin Lua Waited");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        QVERIFY(QDir().mkpath(cacheLocation()));
        const QString lockPathFileName = qsl("%1/mudlet-lua.lock").arg(cacheLocation());
        QLockFile heldElsewhere(lockPathFileName);
        QVERIFY(heldElsewhere.tryLock());
        // From another thread, as this one is waiting for the lock by then
        std::thread letGo([&heldElsewhere] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            heldElsewhere.unlock();
        });
        const auto joined = qScopeGuard([&letGo] {
            letGo.join();
        });

        THeadlessSession session;
        QTest::failOnWarning(QRegularExpression(qsl("another Mudlet still holds")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QCOMPARE(luaValue(session, qsl("luaGlobalPath")), unpackedFolder());
        QVERIFY2(!QFileInfo::exists(lockPathFileName), "The lock is still held once the copy is in place");
    }

    void test_anUnpackedCopyIsReusedUnlessAFileOfItIsMissingOrCutShort()
    {
        const QString name = qsl("Builtin Lua Reused");
        QVERIFY(writeProfile(name));
        QVERIFY(registerBuiltinCopy());
        const auto unregister = qScopeGuard(unregisterBuiltinCopy);
        const QString folder = unpackedFolder();
        const QString keyCodes = qsl("%1/KeyCodes.lua").arg(folder);
        const QString geyserLabel = qsl("%1/geyser/GeyserLabel.lua").arg(folder);
        const QString geyserLabelSource = qsl("@%1/geyser/GeyserLabel.lua").arg(folder);
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QCOMPARE(luaValue(session, qsl("luaGlobalPath")), folder);
        }

        // Only a copy that is read again runs this, written at the file's own size as a copy of another size is not kept
        QByteArray marked("builtinLuaTestCopyWasReused = true\n--");
        marked.append(QFileInfo(keyCodes).size() - marked.size(), 'x');
        QVERIFY(writeFile(keyCodes, marked));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QCOMPARE(luaValue(session, qsl("builtinLuaTestCopyWasReused")), qsl("true"));
        }

        // As the system's cache cleaner may leave it
        QVERIFY(QFile::remove(geyserLabel));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QCOMPARE(luaValue(session, qsl("luaGlobalPath")), folder);
            QCOMPARE(luaValue(session, qsl("debug.getinfo(Geyser.Label.new, 'S').source")), geyserLabelSource);
            QCOMPARE(luaValue(session, qsl("builtinLuaTestCopyWasReused")), qsl("nil"));
        }

        // As a power cut may leave it, which Lua would run without complaint
        QVERIFY(writeFile(geyserLabel));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QCOMPARE(luaValue(session, qsl("luaGlobalPath")), folder);
            QCOMPARE(luaValue(session, qsl("debug.getinfo(Geyser.Label.new, 'S').source")), geyserLabelSource);
        }
    }
};

#include "BuiltinMudletLuaTest.moc"
MUDLET_GROUPED_TEST_MAIN(BuiltinMudletLuaTest)
