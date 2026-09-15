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
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgPackageManager.h"
#include "mudlet.h"

#include "GroupedTest.h"

// The Package Manager removes every package the user selected in one go, and
// an uninstall can be refused for two quite different reasons: a profile save
// is running, or the package is not installed any more. Only the first is
// something the user can wait out. The second one happens within this very
// loop - a package's sysUninstall handler is free to take a sibling away, and
// the rows were drawn before any of that.
//
// A spec cannot reach this: the dialog is C++ only, with no Lua way to open it
// or to run its removal.
class PackageManagerRemovalTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-PackageManagerRemoval";
    QString mpPort;
    const QString mpLocalhost = "localhost";

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

    // The one a sibling's sysUninstall handler took away first. It is gone,
    // which is what the user asked for, so telling them to wait for a save that
    // is not running sends them back to a dialog that will say the same thing
    // for as long as they keep trying.
    void test_aPackageAHandlerRemovedIsNotBlamedOnAProfileSave()
    {
        auto* host = startProfile();
        QVERIFY2(host, "Could not start the profile");

        host->mInstalledPackages << qsl("removal-a") << qsl("removal-b");
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function pkgRemovalSibling(_, name)\n"
                                                                        "  if name == 'removal-a' then uninstallPackage('removal-b') end\n"
                                                                        "end\n"
                                                                        "registerAnonymousEventHandler('sysUninstall', 'pkgRemovalSibling')\n")),
                 "The sysUninstall handler could not be registered");

        auto* manager = new dlgPackageManager(nullptr, host);
        const QString msg = manager->removePackages(QStringList({qsl("removal-a"), qsl("removal-b")}));
        delete manager;

        QVERIFY2(!host->mInstalledPackages.contains(qsl("removal-a")), "SETUP: the package the user named was not removed at all");
        QVERIFY2(!host->mInstalledPackages.contains(qsl("removal-b")), "SETUP: the handler never took the sibling away, so nothing was refused");
        QVERIFY2(!host->currentlySavingProfile(), "SETUP: a profile save really was running, so the save wording would be right");
        QVERIFY2(!msg.isEmpty(), "SETUP: the second removal was not refused, so there is no wording to check");

        QVERIFY2(!msg.contains(qsl("being saved")), qPrintable(qsl("A package a handler had already removed was blamed on a profile save: %1").arg(msg)));
        QVERIFY2(msg.contains(qsl("no longer installed")) && msg.contains(qsl("removal-b")), qPrintable(qsl("The package that had gone was not named as such: %1").arg(msg)));
    }

    // The other half of the same wording: a save really running is still the
    // thing to say, and it names every package it turned away.
    void test_aRemovalRefusedByASaveStillSaysSo()
    {
        auto* host = startProfile();
        QVERIFY2(host, "Could not start the profile");

        host->mInstalledPackages << qsl("removal-c") << qsl("removal-d");
        host->saveProfile();
        QVERIFY2(host->currentlySavingProfile(), "SETUP: the save had already finished, so nothing would be refused for it");

        auto* manager = new dlgPackageManager(nullptr, host);
        const QString msg = manager->removePackages(QStringList({qsl("removal-c"), qsl("removal-d")}));
        delete manager;

        QVERIFY2(msg.contains(qsl("being saved")), qPrintable(qsl("A removal refused by a running save did not say so: %1").arg(msg)));
        QVERIFY2(msg.contains(qsl("removal-c")) && msg.contains(qsl("removal-d")), qPrintable(qsl("Not every package the save turned away was named: %1").arg(msg)));
        QVERIFY2(!msg.contains(qsl("no longer installed")), qPrintable(qsl("A package still installed was reported as gone: %1").arg(msg)));

        host->waitForProfileSave();
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
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "PackageManagerRemovalTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageManagerRemovalTest)
