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

#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <chrono>
#include <zip.h>

#include "AliasUnit.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A module synced between profiles is reloaded in every other profile that has it whenever one of them
// saves, from the copy the saving profile has just written. No Lua call reaches that path - a script
// only ever reloads a module of its own profile - so it is driven from here, with two profiles open.
class ModuleSyncReloadTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    QTemporaryDir mConfigDir;
    QTemporaryDir mArchiveDir;
    QByteArray mSavedXdg;
    const QString mFirstProfile = qsl("ModuleSyncReload-A");
    const QString mSecondProfile = qsl("ModuleSyncReload-B");
    const QString mModuleName = qsl("module-sync-reload");
    const QString mAliasName = qsl("module-sync-reload alias");
    const QString mSyncedAliasName = qsl("module-sync-reload synced alias");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    Host* mpFirstHost = nullptr;
    Host* mpSecondHost = nullptr;

    static QByteArray moduleXml(const QString& aliasName)
    {
        return qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<!DOCTYPE MudletPackage>\n"
                   "<MudletPackage version=\"1.001\">\n"
                   "<AliasPackage>\n"
                   "<Alias isActive=\"yes\" isFolder=\"no\">\n"
                   "<name>%1</name>\n"
                   "<script></script>\n"
                   "<command></command>\n"
                   "<packageName></packageName>\n"
                   "<regex>^module-sync-reload$</regex>\n"
                   "</Alias>\n"
                   "</AliasPackage>\n"
                   "</MudletPackage>\n")
                .arg(aliasName)
                .toUtf8();
    }

    static bool writeArchive(const QString& path, const QString& entryName, const QByteArray& contents)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
        if (!source || zip_file_add(archive, entryName.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
        return zip_close(archive) == 0;
    }

    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        return file.write(contents) == contents.size();
    }

    QString archivePath(const QString& profile) const { return mArchiveDir.filePath(qsl("%1/%2.mpackage").arg(profile, mModuleName)); }
    QString unpackedXmlPath(const QString& profile) const { return MudletApp::getMudletPath(enums::profilePackagePathFileName, profile, mModuleName); }

    static void settleSaves(Host* host)
    {
        for (int i = 0; i < 200 && (host->hasPendingProfileSave() || host->currentlySavingProfile()); ++i) {
            QTest::qWait(20ms);
            host->waitForProfileSave();
        }
    }

    bool installIn(Host* host, const QString& profile)
    {
        QDir().mkpath(mArchiveDir.filePath(profile));
        if (!writeArchive(archivePath(profile), qsl("%1.xml").arg(mModuleName), moduleXml(mAliasName))) {
            return false;
        }
        settleSaves(host);
        auto [installed, message] = host->installPackage(archivePath(profile), enums::PackageModuleType::ModuleFromScript, true);
        settleSaves(host);
        if (!installed || !host->mModulesLoadedOk.contains(mModuleName)) {
            qWarning().noquote() << "installing the module in" << profile << "failed:" << message;
            return false;
        }
        return true;
    }

    void uninstallFrom(Host* host)
    {
        settleSaves(host);
        if (host->mInstalledModules.contains(mModuleName)) {
            host->uninstallPackage(mModuleName, enums::PackageModuleType::ModuleFromScript);
        }
        settleSaves(host);
    }

    static int aliasCount(Host* host, const QString& aliasName) { return static_cast<int>(host->getAliasUnit()->findItems(aliasName, true, true).size()); }

    static QString consoleText(Host* host)
    {
        QPointer<TMainConsole> console = host->mainConsoleView();
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    static QString runLua(Host* host, const QString& code)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(not a string)");
        lua_pop(L, 1);
        return error;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mArchiveDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpFirstHost = TestProfile::create(mFirstProfile, mLocalhost, mPort);
        QVERIFY2(mpFirstHost, "the first profile did not open");
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mSecondProfile)));
        QVERIFY(MudletApp::writeProfileData(mSecondProfile, qsl("url"), mLocalhost).first);
        QVERIFY(MudletApp::writeProfileData(mSecondProfile, qsl("port"), mPort).first);
        QVERIFY2(runLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(mSecondProfile)).isNull(), "the second profile could not be loaded");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return HostManager::self()->getHost(mSecondProfile) != nullptr;
                         },
                         10s),
                 "the second profile did not open");
        mpSecondHost = HostManager::self()->getHost(mSecondProfile);
        QTest::qWait(500ms);
    }

    void init()
    {
        QVERIFY2(installIn(mpFirstHost, mFirstProfile), "could not install the module in the first profile");
        QVERIFY2(installIn(mpSecondHost, mSecondProfile), "could not install the module in the second profile");
        QVERIFY(mpFirstHost->changeModuleSync(mModuleName, QLatin1String("1")).first);
        QVERIFY(mpSecondHost->changeModuleSync(mModuleName, QLatin1String("1")).first);
        settleSaves(mpFirstHost);
        settleSaves(mpSecondHost);
        QCOMPARE(aliasCount(mpSecondHost, mAliasName), 1);
    }

    void cleanup()
    {
        uninstallFrom(mpFirstHost);
        uninstallFrom(mpSecondHost);
    }

    void cleanupTestCase()
    {
        settleSaves(mpFirstHost);
        settleSaves(mpSecondHost);
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // What the other profile has just written is what is read in, not this profile's own archive
    void test_aSyncReadsTheOtherProfilesCopy()
    {
        QVERIFY(writeFile(unpackedXmlPath(mFirstProfile), moduleXml(mSyncedAliasName)));

        auto [reloaded, reason] = mpSecondHost->reloadModule(mModuleName, mFirstProfile);

        QVERIFY2(reloaded, qPrintable(reason));
        QCOMPARE(aliasCount(mpSecondHost, mSyncedAliasName), 1);
        QCOMPARE(aliasCount(mpSecondHost, mAliasName), 0);
        QCOMPARE(mpSecondHost->mInstalledModules.value(mModuleName), (QStringList{archivePath(mSecondProfile), qsl("1")}));
    }

    // The reload takes the module apart before reading the other profile's copy back in, so a copy that has
    // gone would leave this profile's module listed with nothing in it (#10211)
    void test_aCopyThatCannotBeReadLeavesTheModuleAlone()
    {
        QVERIFY(QFile::remove(unpackedXmlPath(mFirstProfile)));

        auto [reloaded, reason] = mpSecondHost->reloadModule(mModuleName, mFirstProfile);

        QVERIFY(!reloaded);
        QVERIFY2(reason.contains(qsl("cannot be read")) && reason.contains(unpackedXmlPath(mFirstProfile)), qPrintable(reason));
        QCOMPARE(aliasCount(mpSecondHost, mAliasName), 1);
        QVERIFY(mpSecondHost->mModulesLoadedOk.contains(mModuleName));
    }

    void test_aCopyThatCannotBeImportedIsReported()
    {
        QVERIFY(writeFile(unpackedXmlPath(mFirstProfile), "this is not a module"));
        const qsizetype consoleBefore = consoleText(mpSecondHost).size();

        auto [reloaded, reason] = mpSecondHost->reloadModule(mModuleName, mFirstProfile);

        QVERIFY(!reloaded);
        QVERIFY2(reason.contains(qsl("could not be loaded")), qPrintable(reason));
        QVERIFY(mpSecondHost->mInstalledModules.contains(mModuleName));
        const QString consoleGained = consoleText(mpSecondHost).mid(consoleBefore);
        QVERIFY2(consoleGained.contains(qsl("Failed to load module \"%1\"").arg(mModuleName)), qPrintable(consoleGained));
    }
};

#include "ModuleSyncReloadTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModuleSyncReloadTest)
