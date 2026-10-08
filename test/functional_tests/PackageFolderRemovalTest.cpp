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

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// uninstallPackage() removes the profile folder named after the package, and the
// name comes from the profile save, which can be hand-edited or older than the
// install-time checks. A name that is not one folder of the profile's own must
// not take anything outside the package with it (#10643). installPackage()
// refuses such names, so only a test that writes mInstalledPackages directly,
// as a loaded save does, can get one into the profile.
class PackageFolderRemovalTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-PackageFolderRemoval";
    const QString mSiblingName = "Test-PackageFolderRemoval-sibling";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    // Only files: removeDir() gives up at the first folder it cannot step into,
    // so a folder holding one would keep its files whatever the uninstall did.
    QString writeCanary(const QString& directory)
    {
        if (!QDir().mkpath(directory)) {
            return QString();
        }
        const QString path = qsl("%1/mudlet-canary.txt").arg(directory);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }
        file.write("not a package's");
        file.close();
        return path;
    }

    void settleSaves(Host* host)
    {
        for (int i = 0; i < 200 && (host->hasPendingProfileSave() || host->currentlySavingProfile()); ++i) {
            QTest::qWait(20ms);
            host->waitForProfileSave();
        }
    }

    QString siblingPath() const { return qsl("%1/%2").arg(MudletApp::getMudletPath(enums::profilesPath), mSiblingName); }

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mpHostname);
        QDir(siblingPath()).removeRecursively();
    }

    void cleanup()
    {
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50ms);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
        TestProfile::removeProfileDirectory(mpHostname);
        QDir(siblingPath()).removeRecursively();
    }

    void test_aNameFromTheSaveThatStepsOutOfTheProfileDeletesNothing_data()
    {
        QTest::addColumn<QString>("packageName");
        QTest::newRow("a sibling of the profile") << qsl("../%1").arg(mSiblingName);
        QTest::newRow("the folder of every profile") << qsl("..");
        QTest::newRow("the profile itself") << qsl(".");
        QTest::newRow("no name at all") << QString();
    }

    void test_aNameFromTheSaveThatStepsOutOfTheProfileDeletesNothing()
    {
        QFETCH(QString, packageName);
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(host, "No active host available for the test.");
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "Could not connect with the host.");

        const QString siblingCanary = writeCanary(siblingPath());
        const QString profileCanary = writeCanary(MudletApp::getMudletPath(enums::profileHomePath, mpHostname));
        QVERIFY2(!siblingCanary.isEmpty() && !profileCanary.isEmpty(), "Could not write the canary files");

        settleSaves(host);
        host->mInstalledPackages.append(packageName);
        QVERIFY2(host->uninstallPackage(packageName, enums::PackageModuleType::Package), "The uninstall was refused");
        QApplication::processEvents();

        QVERIFY2(!host->mInstalledPackages.contains(packageName), "The package stayed listed");
        QVERIFY2(QFileInfo::exists(siblingCanary), "Uninstalling the package deleted files in a folder beside the profile");
        QVERIFY2(QFileInfo::exists(profileCanary), "Uninstalling the package deleted the profile's own files");
    }
};

#include "PackageFolderRemovalTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageFolderRemovalTest)
