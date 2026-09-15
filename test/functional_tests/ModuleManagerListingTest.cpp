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
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgModuleManager.h"
#include "mudlet.h"

#include "GroupedTest.h"

// The Module Manager's table is rebuilt in place, and installPackage() and
// uninstallPackage() rebuild it behind the user's back whenever a module goes
// in or out while the dialog is open. What is listed afterwards has to be what
// is installed - no leftovers of the previous listing, and nothing still shown
// for a module that has gone.
//
// A spec cannot reach this: the dialog is C++ only, with no Lua way to open it
// or to read a row back out of it.
class ModuleManagerListingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-ModuleManagerListing";
    QString mpPort;
    const QString mpLocalhost = "localhost";

    // Four is the smallest number that shows the leftovers: a clear that
    // advances past a row each time it removes one leaves every other row.
    const QStringList mModuleNames{qsl("listing-a"), qsl("listing-b"), qsl("listing-c"), qsl("listing-d")};

    void listModules(Host* host)
    {
        for (const QString& name : mModuleNames) {
            host->mInstalledModules[name] = QStringList{qsl("%1/%2.xml").arg(mudlet::getMudletPath(enums::profileHomePath, mpHostname), name), qsl("0")};
            host->mModulePriorities[name] = 0;
        }
    }

    QStringList rowNames(dlgModuleManager* manager) const
    {
        QStringList names;
        for (int row = 0; row < manager->moduleTable->rowCount(); ++row) {
            auto* item = manager->moduleTable->item(row, 0);
            names << (item ? item->text() : QString());
        }
        return names;
    }

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
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void cleanup()
    {
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

    // Installing a module while the dialog is open rebuilds the table, and a
    // rebuild that does not clear what was there first shows the modules that
    // survived the clear twice over.
    void test_rebuildingTheListingDoesNotLeaveTheOldOneBehind()
    {
        auto* host = startProfile();
        QVERIFY2(host, "Could not start the profile");
        listModules(host);

        auto* manager = new dlgModuleManager(nullptr, host);
        QCOMPARE(rowNames(manager), mModuleNames);

        // what installPackage() and uninstallPackage() do to the open dialog
        manager->layoutModules();
        QCOMPARE(rowNames(manager), mModuleNames);

        delete manager;
    }

    // A module taken away by a script, or by the other dialog, has to leave the
    // listing with it - a row for a module that is not installed offers the
    // user a removal that can only be refused.
    void test_aModuleUninstalledElsewhereLeavesTheListing()
    {
        auto* host = startProfile();
        QVERIFY2(host, "Could not start the profile");
        listModules(host);

        auto* manager = new dlgModuleManager(nullptr, host);
        QCOMPARE(rowNames(manager), mModuleNames);

        QVERIFY2(host->uninstallPackage(qsl("listing-b"), enums::PackageModuleType::ModuleFromScript), "The seeded module could not be uninstalled");
        QVERIFY2(!host->mInstalledModules.contains(qsl("listing-b")), "The module was answered yes but is still listed by the profile");

        QCOMPARE(rowNames(manager), QStringList({qsl("listing-a"), qsl("listing-c"), qsl("listing-d")}));

        delete manager;
    }

private:
    Host* startProfile()
    {
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        if (!host) {
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2000)) {
            return nullptr;
        }
        return mudlet::self()->getActiveHost();
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "ModuleManagerListingTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModuleManagerListingTest)
