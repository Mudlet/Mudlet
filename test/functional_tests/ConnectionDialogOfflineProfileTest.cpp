/***************************************************************************
 *   Copyright (C) 2026 by Michael Conley - sousesider@gmail.com           *
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
 * Whether the connection dialog will open a profile that has no game server
 * address. Mudlet itself opens one perfectly well - "--profile <name> --offline"
 * against a profile with no url does - and the dialog creates such profiles
 * itself, so refusing them left them listed under My games and unreachable, with
 * nothing said: a disabled button cannot show its own tooltip. See issue #10756.
 *
 * Run with: ctest -R ConnectionDialogOfflineProfileTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogOfflineProfileTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // A folder and nothing else, which is what the dialog's own New button
    // leaves behind when the name is committed and the address never filled in
    const QString mAddresslessProfile = qsl("Addressless-ConnDialogOffline");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    void selectTestProfile()
    {
        auto* pDialog = dialog();
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, mAddresslessProfile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The addressless test profile is missing from the games list");
        pDialog->listWidget_profiles->setCurrentItem(items.first());
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), mAddresslessProfile)));
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
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

    void test_offlineOpensAProfileWithNoServerAddress()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();

        QVERIFY2(dialog()->host_name_entry->text().trimmed().isEmpty(), "The test profile has an address after all, so this proves nothing");
        QVERIFY2(dialog()->offline_button->isEnabled(), "A profile with no server address cannot be opened offline");
        QVERIFY2(!dialog()->connect_button->isEnabled(), "Connect was offered for a profile with nowhere to connect to");
    }

    // The reason lived only in the disabled button's tooltip, and Qt does not
    // deliver a tooltip to a disabled widget - so the dialog said nothing at all
    void test_theMissingAddressIsExplainedWhereItCanBeRead()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();

        QVERIFY2(dialog()->notificationArea->isVisible(), "Nothing on screen says why this profile cannot be connected to");
        QVERIFY2(!dialog()->notificationAreaMessageBox->text().trimmed().isEmpty(), "The notification area is showing, but with no text in it");
    }

    // Typing an address and taking it away again goes through a different path
    // from selecting the profile, and that one dimmed both buttons by hand
    void test_clearingTheAddressLeavesOfflineAvailable()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();

        dialog()->host_name_entry->setText(qsl("localhost"));
        QVERIFY2(dialog()->connect_button->isEnabled(), "An address was entered and Connect stayed dimmed");
        QVERIFY2(dialog()->offline_button->isEnabled(), "An address was entered and Offline stayed dimmed");

        dialog()->host_name_entry->clear();
        QVERIFY2(!dialog()->connect_button->isEnabled(), "Connect stayed available with the address taken away");
        QVERIFY2(dialog()->offline_button->isEnabled(), "Taking the address away took the Offline button with it");
    }
};

MUDLET_GROUPED_TEST_MAIN(ConnectionDialogOfflineProfileTest)
#include "ConnectionDialogOfflineProfileTest.moc"
