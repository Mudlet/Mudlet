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
 * The package repository index is keyed by package name alone, and anyone can
 * make a package with the name of one listed there. The Package Manager only
 * offers the repository's "report an issue" and website links, and the
 * repository's help URL, for an installed package that is the repository's:
 * one whose author matches the index entry of that name (#8894).
 *
 * A spec cannot reach this: the dialog is C++ only, with no Lua way to open it
 * or to read its buttons.
 *
 * Run with: ctest -R PackageManagerRepositoryLinksTest -V
 */

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QListWidget>
#include <QPushButton>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgPackageManager.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PackageManagerRepositoryLinksTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgPackageManager* mpManager = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("Test-PackageManagerRepositoryLinks");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    const QString mPackageName = qsl("PRS");
    const QString mRepositoryAuthor = qsl("Stack");
    const QString mRepositoryHelpUrl = qsl("https://example.org/repository-help");

    QString profileHome() const { return MudletApp::getMudletPath(enums::profileHomePath, mProfileName); }

    // Read by the dialog's constructor, so no index download is ever started
    bool writeRepositoryIndex() const
    {
        QJsonObject entry;
        entry.insert(qsl("mpackage"), mPackageName);
        entry.insert(qsl("filename"), qsl("PRS.mpackage"));
        entry.insert(qsl("title"), qsl("The repository's PRS"));
        entry.insert(qsl("author"), mRepositoryAuthor);
        entry.insert(qsl("version"), qsl("1.8.0"));
        entry.insert(qsl("description"), qsl("a package listed in the repository index"));
        entry.insert(qsl("helpURL"), mRepositoryHelpUrl);
        QJsonObject root;
        root.insert(qsl("packages"), QJsonArray{entry});

        QFile file(qsl("%1/mpkg.packages.json").arg(profileHome()));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        return file.write(QJsonDocument(root).toJson()) > 0;
    }

    void installPackage(const QString& author, const QString& helpUrl = QString()) const
    {
        mpHost->mInstalledPackages << mPackageName;
        QMap<QString, QString> info{{qsl("mpackage"), mPackageName}, {qsl("title"), qsl("A package of the same name")}, {qsl("author"), author}, {qsl("version"), qsl("1.0")}};
        if (!helpUrl.isEmpty()) {
            info.insert(qsl("helpURL"), helpUrl);
        }
        mpHost->mPackageInfo.insert(mPackageName, info);
    }

    void openManager()
    {
        QVERIFY(writeRepositoryIndex());
        mpManager = new dlgPackageManager(nullptr, mpHost);
        mpManager->resize(900, 600);
        mpManager->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpManager));
    }

    void selectPackage()
    {
        const auto rows = mpManager->packageList->findItems(mPackageName, Qt::MatchExactly);
        QVERIFY2(!rows.isEmpty(), "SETUP: the package is not listed in this view");
        mpManager->packageList->setCurrentItem(rows.first());
        QCOMPARE(mpManager->packageList->currentItem()->text(), mPackageName);
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
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(profileHome()).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // a failed QVERIFY leaves its slot at once, so a shown dialog and the
    // seeded package would otherwise carry over into the next case
    void cleanup()
    {
        delete mpManager;
        mpManager = nullptr;
        if (mpHost) {
            mpHost->mInstalledPackages.removeAll(mPackageName);
            mpHost->mPackageInfo.remove(mPackageName);
        }
    }

    void test_aSameNamedPackageByAnotherAuthorGetsNoRepositoryLinks()
    {
        installPackage(qsl("Someone Else"));
        openManager();
        selectPackage();

        QVERIFY2(mpManager->pushButton_report->isHidden(), "a package that only shares its name with a repository one offers to report an issue to the repository");
        QVERIFY2(mpManager->pushButton_website->isHidden(), "a package that only shares its name with a repository one links to the repository's website");
    }

    void test_theRepositoryPackageGetsItsLinks()
    {
        installPackage(mRepositoryAuthor);
        openManager();
        selectPackage();

        QVERIFY2(!mpManager->pushButton_report->isHidden(), "the repository's own package lost its report button");
        QVERIFY2(!mpManager->pushButton_website->isHidden(), "the repository's own package lost its website button");
        QCOMPARE(mpManager->packageHelpUrl(mPackageName), mRepositoryHelpUrl);
    }

    void test_theRepositoryPackagesOwnHelpUrlStillComesFirst()
    {
        const QString ownHelpUrl = qsl("https://example.org/own-help");
        installPackage(mRepositoryAuthor, ownHelpUrl);
        openManager();

        mpManager->pushButton_explore->click();
        selectPackage();
        QVERIFY2(!mpManager->pushButton_website->isHidden(), "SETUP: the Explore view shows no website button for a listed package");
        QCOMPARE(mpManager->packageHelpUrl(mPackageName), ownHelpUrl);
    }

    // the other way round: the repository's package, browsed in the Explore
    // view, must not send the user to the help URL of an unrelated local one
    void test_theRepositoryWebsiteIgnoresASameNamedPackagesHelpUrl()
    {
        installPackage(qsl("Someone Else"), qsl("https://example.org/someone-elses-help"));
        openManager();

        mpManager->pushButton_explore->click();
        selectPackage();
        QVERIFY2(!mpManager->pushButton_website->isHidden(), "SETUP: the Explore view shows no website button for a listed package");
        QCOMPARE(mpManager->packageHelpUrl(mPackageName), mRepositoryHelpUrl);
    }
};

#include "PackageManagerRepositoryLinksTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageManagerRepositoryLinksTest)
