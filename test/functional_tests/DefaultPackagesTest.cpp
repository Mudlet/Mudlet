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
 * Covers the packages Mudlet preinstalls into new profiles, which live in
 * src/packages and are compiled in through mudlet.qrc.
 *
 * Nothing else notices when one of these breaks: a path in
 * setupPreInstallPackages() that no longer names a compiled-in resource, or an
 * archive rebuilt without config.lua, just means the profile quietly comes up
 * without the package. The build stays green either way, so this test walks
 * both the preinstall table and every archive in the resource tree.
 *
 * Run with: ctest -R DefaultPackagesTest -V
 */

#include <QtTest/QtTest>

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QXmlStreamReader>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

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

class DefaultPackagesTest : public QObject
{
    Q_OBJECT

private:
    // A package installs under the name its config.lua declares, which is
    // normally the directory it lives in. These two are deliberately not: the
    // tutorial uses a display name, and the Carrion Fields loader has to match
    // the name its own script passes to uninstallPackage() when it is done.
    inline static const QHash<QString, QString> scmInstallsAs = {{qsl("mudlet-tutorial"), qsl("Mudlet Tutorial")}, {qsl("CF-loader"), qsl("CF_Loader")}};

    const QString mProfileName = qsl("DefaultPackages-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

    QStringList preinstallsFor(const QString& gameUrl, const QString& profileName = qsl("test"))
    {
        mudlet::self()->mPackagesToInstallList.clear();
        mudlet::self()->setupPreInstallPackages(gameUrl, profileName);
        return mudlet::self()->mPackagesToInstallList;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // The half of the mpkg gate that lives outside the code under test: ctest hands
        // this to every functional test, and test_testModeLeavesOutTheSelfUpdatingPackage
        // below sets it by hand, so nothing else here would notice it going missing. It
        // would go unnoticed for a long time too - the skip only changes an outcome while
        // the repository is ahead of the bundled mpkg, so CI would stay green until the
        // next upstream release and then break somewhere else entirely.
        QVERIFY2(qEnvironmentVariableIsSet("MUDLET_TEST_MODE"), "MUDLET_TEST_MODE is not set - run through ctest, which sets it for every functional test (test/functional_tests/CMakeLists.txt)");

        // Keep the test hermetic: point the config dir resolution at a
        // temporary directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(mudlet::getQSettings()->allKeys().isEmpty(), "a fresh config dir must start out with an empty Mudlet.ini - something wrote settings before init()");
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()), "failed to create the Host");
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Every path the preinstall table hands out has to name something that was
    // actually compiled into the binary, or the profile silently misses it.
    void test_preinstalledPathsResolve_data()
    {
        QTest::addColumn<QString>("gameUrl");

        QTest::newRow("any game") << qsl("example.com");
        QTest::newRow("carrion fields") << qsl("carrionfields.net");
        QTest::newRow("icesus") << qsl("icesus.org");
        QTest::newRow("morgengrauen") << qsl("mg.mud.de");
        QTest::newRow("medievia") << qsl("medievia.com");
        QTest::newRow("an IRE game") << qsl("achaea.com");
        QTest::newRow("mudlet's own") << qsl("mudlet.org");
    }

    void test_preinstalledPathsResolve()
    {
        QFETCH(QString, gameUrl);

        const QStringList paths = preinstallsFor(gameUrl);
        QVERIFY2(!paths.isEmpty(), qPrintable(qsl("no packages queued for %1").arg(gameUrl)));
        for (const QString& path : paths) {
            QVERIFY2(path.startsWith(qsl(":/")), qPrintable(qsl("%1 is not a resource path").arg(path)));
            QVERIFY2(QFile::exists(path), qPrintable(qsl("%1 is queued for %2 but is not compiled in").arg(path, gameUrl)));
        }
    }

    void test_tutorialProfileGetsTheTutorial()
    {
        QVERIFY(preinstallsFor(qsl("localhost"), qsl("Mudlet Tutorial")).contains(qsl(":/packages/mudlet-tutorial/mudlet-tutorial.mpackage")));
        QVERIFY(!preinstallsFor(qsl("localhost"), qsl("some other profile")).contains(qsl(":/packages/mudlet-tutorial/mudlet-tutorial.mpackage")));
    }

    // Games that install an interface of their own get a loader instead of the
    // starter UI, which would otherwise fight it for the same screen space.
    void test_gamesWithTheirOwnUiSkipTheStarterUi()
    {
        QVERIFY(preinstallsFor(qsl("example.com")).contains(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage")));
        // mg.mud.de is one of MorgenGrauen's alternate addresses and icesus.org
        // is Icesus' primary one, so both halves of the URL match are covered.
        QVERIFY(!preinstallsFor(qsl("mg.mud.de")).contains(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage")));
        QVERIFY(preinstallsFor(qsl("mg.mud.de")).contains(qsl(":/packages/mg-loader/mg-loader.mpackage")));
        QVERIFY(!preinstallsFor(qsl("icesus.org")).contains(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage")));
    }

    // Why mpkg cannot be in a test profile: see setupPreInstallPackages() in
    // src/mudlet.cpp. Both directions are asserted, so removing the skip and leaving it
    // permanently on both fail, and the players' half pins the resource path the skip
    // matches on - rename it in the table alone and the first check goes red.
    void test_testModeLeavesOutTheSelfUpdatingPackage()
    {
        const QString mpkg = qsl(":/packages/mpkg/mpkg.mpackage");
        const QByteArray savedTestMode = qgetenv("MUDLET_TEST_MODE");
        const auto restoreTestMode = qScopeGuard([&savedTestMode]() {
            savedTestMode.isNull() ? qunsetenv("MUDLET_TEST_MODE") : qputenv("MUDLET_TEST_MODE", savedTestMode);
        });

        qunsetenv("MUDLET_TEST_MODE");
        const QStringList forPlayers = preinstallsFor(qsl("example.com"));
        QVERIFY2(forPlayers.contains(mpkg), "players are meant to get mpkg - only tests go without it");
        QVERIFY2(QFile::exists(mpkg), "mpkg is queued for every profile but is not compiled in");

        qputenv("MUDLET_TEST_MODE", "1");
        const QStringList forTests = preinstallsFor(qsl("example.com"));

        // The whole difference, not just mpkg's absence: a skip that caught more than it
        // meant to would otherwise pass, and the packages it silently dropped are the
        // ones the other tests in this file build their profiles from.
        QStringList dropped = forPlayers;
        for (const QString& package : forTests) {
            dropped.removeOne(package);
        }
        QCOMPARE(dropped, QStringList{mpkg});
    }

    // The generic mapper is for games that have no mapper script of their own.
    void test_ireGamesGetTheirOwnMapper()
    {
        QVERIFY(preinstallsFor(qsl("achaea.com")).contains(qsl(":/mudlet-mapper.xml")));
        QVERIFY(!preinstallsFor(qsl("achaea.com")).contains(qsl(":/packages/generic_mapper/generic_mapper.mpackage")));
        QVERIFY(preinstallsFor(qsl("example.com")).contains(qsl(":/packages/generic_mapper/generic_mapper.mpackage")));
    }

    // Installing is what the preinstall table ultimately does, and a package
    // that unpacks but does not import leaves the profile just as empty.
    void test_packagesInstall_data()
    {
        QTest::addColumn<QString>("package");

        for (const QString& package : QDir(qsl(":/packages")).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QTest::newRow(qPrintable(package)) << package;
        }
    }

    void test_packagesInstall()
    {
        QFETCH(QString, package);

        mpHost->mBlockScriptCompile = false;
        auto [installed, message] = mpHost->installPackage(qsl(":/packages/%1/%1.mpackage").arg(package), enums::PackageModuleType::Package, true);
        QVERIFY2(installed, qPrintable(qsl("%1 failed to install: %2").arg(package, message)));

        const QString installedAs = scmInstallsAs.value(package, package);
        QVERIFY2(mpHost->mInstalledPackages.contains(installedAs), qPrintable(qsl("%1 installed but is not registered as %2").arg(package, installedAs)));
    }

    // Each package directory carries the archive Mudlet installs. Unpack every
    // one the way Host::installPackage() does and check it is shaped the way
    // the installer requires: metadata in config.lua, exactly one xml.
    void test_everyArchiveIsWellFormed()
    {
        const QStringList packages = QDir(qsl(":/packages")).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QVERIFY2(!packages.isEmpty(), "no packages found in the resource tree");

        for (const QString& package : packages) {
            const QString archive = qsl(":/packages/%1/%1.mpackage").arg(package);
            QVERIFY2(QFile::exists(archive), qPrintable(qsl("%1 holds no archive named after it").arg(package)));

            QTemporaryFile onDisk;
            QVERIFY(onDisk.open());
            QFile resource(archive);
            QVERIFY(resource.open(QIODevice::ReadOnly));
            QVERIFY(onDisk.write(resource.readAll()) != -1);
            onDisk.close();

            QTemporaryDir unpacked;
            QVERIFY(unpacked.isValid());
            // mudlet::unzip() joins the destination and the entry name as-is,
            // so the trailing slash is what keeps the files inside the folder:
            const QString destination = qsl("%1/").arg(unpacked.path());
            QVERIFY2(mudlet::unzip(onDisk.fileName(), destination, QDir(unpacked.path())), qPrintable(qsl("%1 could not be unzipped").arg(archive)));

            const QDir contents(unpacked.path());
            QVERIFY2(contents.exists(qsl("config.lua")), qPrintable(qsl("%1 carries no config.lua, so it would install without any metadata").arg(archive)));
            const QStringList xmls = contents.entryList(QStringList{qsl("*.xml")}, QDir::Files);
            QCOMPARE(xmls.count(), 1);

            // Mudlet names the installed package after config.lua, so keeping
            // the directory named the same is what makes the paths guessable.
            const QString declaredName = mpHost->getPackageConfig(contents.absoluteFilePath(qsl("config.lua")));
            QVERIFY2(!declaredName.isEmpty(), qPrintable(qsl("%1 declares no package name in its config.lua").arg(archive)));
            QCOMPARE(declaredName, scmInstallsAs.value(package, package));
        }
    }

    // A package's Lua lives inside its XML, entity-escaped, and is only compiled
    // when a profile installs the package - so a typo in it reaches users rather
    // than CI. Compile every script body here instead.
    void test_everyPackageScriptCompiles()
    {
        const QStringList packages = QDir(qsl(":/packages")).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QVERIFY2(!packages.isEmpty(), "no packages found in the resource tree");

        lua_State* L = luaL_newstate();
        QVERIFY(L);
        auto closeState = qScopeGuard([L]() {
            lua_close(L);
        });

        int compiled = 0;
        for (const QString& package : packages) {
            const QString archive = qsl(":/packages/%1/%1.mpackage").arg(package);
            QTemporaryFile onDisk;
            QVERIFY(onDisk.open());
            QFile resource(archive);
            QVERIFY(resource.open(QIODevice::ReadOnly));
            QVERIFY(onDisk.write(resource.readAll()) != -1);
            onDisk.close();

            QTemporaryDir unpacked;
            QVERIFY(unpacked.isValid());
            const QString destination = qsl("%1/").arg(unpacked.path());
            QVERIFY(mudlet::unzip(onDisk.fileName(), destination, QDir(unpacked.path())));

            // the installer imports every *.xml and *.trigger it finds in an
            // archive, so compile the scripts in all of them rather than assuming
            // a package carries one document
            const QDir contents(unpacked.path());
            const QStringList documents = contents.entryList(QStringList{qsl("*.xml"), qsl("*.trigger")}, QDir::Files);
            QVERIFY2(!documents.isEmpty(), qPrintable(qsl("%1 carries nothing the installer would import").arg(package)));

            for (const QString& document : documents) {
                QFile xml(contents.absoluteFilePath(document));
                QVERIFY(xml.open(QIODevice::ReadOnly));
                QXmlStreamReader reader(&xml);
                while (!reader.atEnd()) {
                    if (reader.readNext() != QXmlStreamReader::StartElement || reader.name() != QLatin1String("script")) {
                        continue;
                    }
                    // a folder's own script element is empty, which compiles to nothing
                    const QString code = reader.readElementText();
                    if (code.trimmed().isEmpty()) {
                        continue;
                    }
                    // load only: running these would register handlers and start
                    // downloads, and a package script is not written to survive
                    // being executed outside the profile that installed it
                    const QByteArray chunk = code.toUtf8();
                    const QByteArray name = qsl("@%1/%2").arg(package, document).toUtf8();
                    const int loaded = luaL_loadbuffer(L, chunk.constData(), chunk.size(), name.constData());
                    const QString error = loaded ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
                    lua_settop(L, 0);
                    QVERIFY2(loaded == 0, qPrintable(qsl("%1 carries a script that does not compile: %2").arg(document, error)));
                    ++compiled;
                }
                QVERIFY2(!reader.hasError(), qPrintable(qsl("%1 is not well-formed XML: %2").arg(document, reader.errorString())));
            }
        }
        QVERIFY2(compiled > 0, "no package scripts were found to compile, so this test proved nothing");
    }
};

#include "DefaultPackagesTest.moc"
MUDLET_GROUPED_TEST_MAIN(DefaultPackagesTest)
