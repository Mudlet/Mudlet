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
 * A game that sends Client.GUI again mid-download - a reconnect does - gets a
 * fresh progress dialog, and the superseded one is closed and deleted. Qt's
 * style sheet support caches every styled widget and evicts it on the
 * widget's destroyed() signal; a wildcard disconnect() on the old dialog
 * severs that too, so the cache holds a freed widget and the next application
 * style sheet a script sets with setAppStyleSheet() walks it. Qt warns about
 * exactly that wildcard at the disconnect, which is what this test turns into
 * a failure; under a sanitiser the restyle itself fails.
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TMainConsole.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <QPointer>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class PackageDownloadDialogRestyleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("PackageDownloadDialogRestyle-Test");

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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        mudlet::self()->addConsoleForNewHost(mpHost);
        QVERIFY(mpHost->mpConsole);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        }
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aSupersededDownloadDialogLeavesTheStyleSheetCaches()
    {
        // Only an application style sheet makes the later restyle walk the caches
        qApp->setStyleSheet(qsl("QProgressDialog { color: #123456; }"));
        const auto clearStyleSheet = qScopeGuard([]() {
            qApp->setStyleSheet(QString());
        });
        QTest::failOnWarning(QRegularExpression(qsl("wildcard call disconnects from destroyed signal")));

        TMainConsole* console = mpHost->mpConsole;
        console->showPackageDownloadProgress(qsl("Downloading the game's UI"), qsl("Cancel"));
        QCOMPARE(console->findChildren<QProgressDialog*>().size(), 1);
        QPointer<QProgressDialog> first = console->findChild<QProgressDialog*>();
        QVERIFY(first);
        QVERIFY(first->isVisible());
        QCoreApplication::processEvents();

        // Stands in for the connection Qt's style sheet support keeps on destroyed()
        bool destroyedSeen = false;
        connect(first, &QObject::destroyed, this, [&destroyedSeen]() {
            destroyedSeen = true;
        });

        console->showPackageDownloadProgress(qsl("Downloading the game's UI again"), qsl("Cancel"));
        QTRY_VERIFY2(first.isNull(), "the superseded dialog was not deleted");
        QVERIFY2(destroyedSeen, "the superseded dialog's destroyed() receivers were severed with it");

        // Restyling walks every widget the caches still hold
        qApp->setStyleSheet(qsl("QProgressDialog { color: #654321; }"));
        QCoreApplication::processEvents();
    }
};

#include "PackageDownloadDialogRestyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageDownloadDialogRestyleTest)
