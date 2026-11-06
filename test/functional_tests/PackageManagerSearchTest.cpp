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
 * The Package Manager's search box filters the list of installed packages. It
 * has to filter the same list the dialog shows with an empty box, under the
 * same names: a package installed without a config.lua has no details but is
 * still installed, and a package is installed under its sanitized name even
 * when its config.lua spelled it with a file ending.
 *
 * A spec cannot reach this: the dialog is C++ only, with no Lua way to open it.
 *
 * Run with: ctest -R PackageManagerSearchTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QLineEdit>
#include <QListWidget>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgPackageManager.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PackageManagerSearchTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgPackageManager* mpManager = nullptr;
    const QString mProfileName = qsl("Test-PackageManagerSearch");
    const QString mLocalhost = qsl("localhost");
    Host* mpHost = nullptr;

    // as a plain .xml package with no config.lua is installed
    const QString mBarePackage = qsl("SearchBarePackage");
    // as one whose config.lua says "mpackage = [[SearchNamedPackage.mpackage]]" is installed
    const QString mNamedPackage = qsl("SearchNamedPackage");

    QStringList listedNames() const
    {
        QStringList names;
        for (int row = 0; row < mpManager->packageList->count(); ++row) {
            names << mpManager->packageList->item(row)->text();
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host after profile creation");

        // With no repository index on disk the dialog downloads one each time it opens
        QFile index(MudletApp::getMudletPath(enums::profileHomePath, mProfileName + QDir::separator() + qsl("mpkg.packages.json")));
        QVERIFY(index.open(QIODevice::WriteOnly));
        QVERIFY(index.write(R"({"packages": []})") > 0);
        index.close();

        mpHost->mInstalledPackages << mBarePackage << mNamedPackage;
        mpHost->mPackageInfo.insert(mNamedPackage, {{qsl("mpackage"), qsl("%1.mpackage").arg(mNamedPackage)}, {qsl("title"), qsl("A titled package")}});
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        mpManager = new dlgPackageManager(nullptr, mpHost);
        mpManager->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpManager));
        QVERIFY2(listedNames().contains(mBarePackage), "SETUP: the package with no details is not listed before any search");
    }

    void cleanup()
    {
        delete mpManager;
        mpManager = nullptr;
    }

    void test_aPackageWithNoDetailsIsFoundByName()
    {
        mpManager->lineEdit_searchBar->setText(qsl("barepack"));
        QCOMPARE(listedNames(), QStringList{mBarePackage});
    }

    void test_clearingTheSearchListsEveryInstalledPackageAgain()
    {
        const QStringList before = listedNames();
        mpManager->lineEdit_searchBar->setText(qsl("x"));
        mpManager->lineEdit_searchBar->clear();
        QCOMPARE(listedNames(), before);
    }

    void test_aFoundPackageIsListedUnderItsInstalledName()
    {
        mpManager->lineEdit_searchBar->setText(qsl("titled"));
        QCOMPARE(listedNames(), QStringList{mNamedPackage});
    }
};

#include "PackageManagerSearchTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageManagerSearchTest)
