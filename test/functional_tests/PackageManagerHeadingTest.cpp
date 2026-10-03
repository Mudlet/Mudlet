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
 * The Package Manager shortens the package name in its heading to what the
 * heading can show. The first package is filled in while the dialog is still
 * being put together, before any layout has given that label its width - so
 * measuring against the width it has at that moment cut a name that had the
 * whole width of the details card to itself down to a couple of characters.
 * The name is kept and measured again every time the heading is given a new
 * width, so widening the dialog gives more of the name back.
 *
 * A spec cannot reach this: the dialog is C++ only, with no Lua way to open it
 * or to read its heading.
 *
 * Run with: ctest -R PackageManagerHeadingTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgPackageManager.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PackageManagerHeadingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgPackageManager* mpManager = nullptr;
    const QString mProfileName = qsl("Test-PackageManagerHeading");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    // sorts first in the list (QString order), so it is the package the dialog
    // opens on even if the profile ever gains others
    const QString mPackageName = qsl("AptlyNamedPackage");

    Host* mpHost = nullptr;

    // the dialog fills its details pane only for a package the profile has
    // details of, so a bare entry in mInstalledPackages would never reach the
    // heading at all
    void installPackage(const QString& name) const
    {
        if (mpHost->mInstalledPackages.contains(name)) {
            return;
        }
        mpHost->mInstalledPackages << name;
        mpHost->mPackageInfo.insert(name, {{qsl("title"), qsl("A package for this test")}, {qsl("author"), qsl("Mudlet")}, {qsl("version"), qsl("1")}});
    }

    void openManager()
    {
        mpManager = new dlgPackageManager(nullptr, mpHost);
        mpManager->resize(900, 600);
        mpManager->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpManager));
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
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
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
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // a failed QVERIFY leaves its slot at once, so the dialog is taken down
    // here rather than at the end of each case - a shown window left behind
    // turns one failure into a second, unrelated one in the next case
    void cleanup()
    {
        delete mpManager;
        mpManager = nullptr;
    }

    void test_theFirstPackageOpenedShowsItsWholeName()
    {
        installPackage(mPackageName);
        openManager();

        auto* pCurrentItem = mpManager->packageList->currentItem();
        QVERIFY2(pCurrentItem, "the dialog opened with no package selected");
        QCOMPARE(pCurrentItem->text(), mPackageName);

        auto* pHeading = mpManager->label_packageName;
        const QFontMetrics metrics(pHeading->font());
        QVERIFY2(metrics.horizontalAdvance(mPackageName) <= pHeading->contentsRect().width(),
                 qPrintable(qsl("SETUP: '%1' needs %2px but the heading is only %3px wide, so shortening it would be right")
                                    .arg(mPackageName, QString::number(metrics.horizontalAdvance(mPackageName)), QString::number(pHeading->contentsRect().width()))));
        QVERIFY2(pHeading->text() == mPackageName, qPrintable(qsl("the heading shows '%1' where the whole of '%2' fits").arg(pHeading->text(), mPackageName)));
        QVERIFY2(pHeading->toolTip().isEmpty(), "a name shown in full does not need a tooltip repeating it");
    }

    // A name too long for the heading still has to be shortened: an un-elided
    // QLabel reports the whole name as its minimum width and drags the dialog's
    // minimum out with it. This case passes without the fix too - it guards the
    // shortening that the case above restores, rather than reproducing the bug.
    void test_aNameTooLongForTheHeadingIsShortened()
    {
        const QString longName = mPackageName.repeated(4);
        installPackage(longName);
        openManager();

        auto* pHeading = mpManager->label_packageName;
        const QFontMetrics metrics(pHeading->font());
        QVERIFY2(metrics.horizontalAdvance(longName) > pHeading->contentsRect().width(), "SETUP: the deliberately over-long name fits after all");

        const auto rows = mpManager->packageList->findItems(longName, Qt::MatchExactly);
        QVERIFY2(!rows.isEmpty(), "the over-long package was not listed at all");
        mpManager->packageList->setCurrentItem(rows.first());
        QVERIFY2(!pHeading->text().isEmpty(), "the heading was blanked rather than shortened");
        QVERIFY2(pHeading->text() != longName, "an over-long package name was not shortened");
        QVERIFY2(longName.startsWith(pHeading->text().chopped(1)), qPrintable(qsl("the heading shows '%1', which is not the start of '%2'").arg(pHeading->text(), longName)));
        QVERIFY2(metrics.horizontalAdvance(pHeading->text()) <= pHeading->contentsRect().width(), "the shortened package name still does not fit the heading");
        QCOMPARE(pHeading->toolTip(), longName);
    }

    // the point of measuring the name again rather than once: the heading is
    // given a new width every time the dialog is resized
    void test_wideningTheDialogGivesMoreOfTheNameBack()
    {
        const QString longName = mPackageName.repeated(4);
        installPackage(longName);
        openManager();

        auto* pHeading = mpManager->label_packageName;
        const auto rows = mpManager->packageList->findItems(longName, Qt::MatchExactly);
        QVERIFY2(!rows.isEmpty(), "the over-long package was not listed at all");
        mpManager->packageList->setCurrentItem(rows.first());
        const QString atNineHundred = pHeading->text();
        QVERIFY2(atNineHundred != longName, "SETUP: the over-long name already fits at 900px, so widening can give nothing back");

        mpManager->resize(1600, 600);
        QTRY_VERIFY2(pHeading->text().length() > atNineHundred.length(), "widening the dialog did not give the heading back more of the name");
        const QFontMetrics metrics(pHeading->font());
        QVERIFY2(metrics.horizontalAdvance(pHeading->text()) <= pHeading->contentsRect().width(), "the re-measured name does not fit the widened heading");

        mpManager->resize(900, 600);
        QTRY_COMPARE(pHeading->text(), atNineHundred);
    }

    // a heading with no width to measure the name against keeps what it is
    // showing: shortening a name to nothing reads as "no package selected"
    void test_aHeadingSqueezedToNothingIsNotBlanked()
    {
        installPackage(mPackageName);
        openManager();
        QCOMPARE(mpManager->label_packageName->text(), mPackageName);

        mpManager->label_packageName->setFixedWidth(0);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
        QVERIFY2(mpManager->label_packageName->contentsRect().width() <= 0, "SETUP: the heading kept a width, so it was never asked to shorten the name to nothing");
        QVERIFY2(!mpManager->label_packageName->text().isEmpty(), "the heading was blanked when the label was squeezed to nothing");
    }

    // clearPackageDetails() drops the kept name as well as the heading, or the
    // next resize would paint the last package's name over an empty pane
    void test_aClearedHeadingStaysClearedThroughAResize()
    {
        installPackage(mPackageName);
        openManager();
        QCOMPARE(mpManager->label_packageName->text(), mPackageName);

        mpManager->pushButton_updates->click();
        QVERIFY2(mpManager->label_packageName->text().isEmpty(), "SETUP: the Updates view had something to show, so nothing was cleared");

        mpManager->resize(1200, 600);
        QTRY_COMPARE(mpManager->label_packageName->text(), QString());
        QVERIFY2(mpManager->label_packageName->toolTip().isEmpty(), "the cleared heading kept a tooltip naming the package that was showing");
    }
};

#include "PackageManagerHeadingTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageManagerHeadingTest)
