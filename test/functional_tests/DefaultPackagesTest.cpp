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

#include <QTemporaryDir>
#include <QTemporaryFile>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForDefaultPackagesTest();

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
        initializeQRCResourcesForDefaultPackagesTest();

        // Keep the test hermetic: point the config dir resolution at a
        // temporary directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
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
        QVERIFY(!preinstallsFor(qsl("mg.mud.de")).contains(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage")));
        QVERIFY(preinstallsFor(qsl("mg.mud.de")).contains(qsl(":/packages/mg-loader/mg-loader.mpackage")));
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
};

void initializeQRCResourcesForDefaultPackagesTest()
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

#include "DefaultPackagesTest.moc"
QTEST_MAIN(DefaultPackagesTest)
