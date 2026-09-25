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
 * A new profile carries a placeholder name until its own is committed, and it
 * gets its folder on disk only then - so an address and port typed before that
 * are written under the placeholder, into a folder that is not there yet, and
 * quietly dropped. Committing the name has to write them out again, which is
 * what PR #6805 added; without it a profile filled in from the bottom up opened
 * with no server address. See issue #2919.
 *
 * Run with: ctest -R ConnectionDialogNewProfileTest -V
 */

#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLineEdit>

#include "GroupedTest.h"

class ConnectionDialogNewProfileTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    const QString mProfileName = qsl("ConnDialogNewProfile-Test");
    const QString mProfileUrl = qsl("mudlet.org");
    const QString mProfilePort = qsl("4000");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    QString profileFolder() const { return MudletPaths::getMudletPath(enums::profileHomePath, mProfileName); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletPaths::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5000));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    // A profile filled in from the bottom up: address and port first, name
    // last. PR #6805.
    void test_theAddressTypedBeforeTheNameIsSaved()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        pDialog->slot_addProfile();
        const QString placeholderName = pDialog->profile_name_entry->text();
        pDialog->host_name_entry->setText(mProfileUrl);
        pDialog->port_entry->setText(mProfilePort);
        // the writes above go under the placeholder name, and are only dropped
        // while that profile has no folder - if one ever appears the rename
        // below carries them over and this case stops proving anything
        QVERIFY2(!QDir(MudletPaths::getMudletPath(enums::profileHomePath, placeholderName)).exists(), "The placeholder profile has a folder on disk, so the writes above had somewhere to go");
        QVERIFY2(pDialog->readProfileData(placeholderName, qsl("url")).isEmpty(), "The address reached disk before the name was committed, so there is nothing left to redo");

        pDialog->profile_name_entry->setText(mProfileName);
        pDialog->slot_saveName();

        QVERIFY2(QDir(profileFolder()).exists(), "Committing the name did not create the profile folder");
        QCOMPARE(pDialog->readProfileData(mProfileName, qsl("url")), mProfileUrl);
        QCOMPARE(pDialog->readProfileData(mProfileName, qsl("port")), mProfilePort);
        QCOMPARE(pDialog->readProfileData(mProfileName, qsl("ssl_tsl")), qsl("0"));
    }
};

#include "ConnectionDialogNewProfileTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogNewProfileTest)
