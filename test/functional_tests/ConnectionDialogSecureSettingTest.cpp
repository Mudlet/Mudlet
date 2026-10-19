/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * Whether a secure connection choice made in Preferences survives the next
 * connect through the connection dialog, which seeds its own Secure checkbox
 * from the profile's ssl_tsl file and hands that to the profile on connect.
 *
 * Run with: ctest -R ConnectionDialogSecureSettingTest -V
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "SettingsTestHelper.h"
#include "TMainConsole.h"
#include "dlgConnectionProfiles.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalSpy>
#include <chrono>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogSecureSettingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    const QString mProfile = qsl("ConnDialogSecure-Test");
    const QString mUnwritableProfile = qsl("ConnDialogSecureUnwritable-Test");
    // a preinstalled game whose default is a secure connection
    const QString mTlsGame = qsl("StickMUD");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    // A dialog asked for while the last profile closes is shown on a timer, and
    // one that loaded a profile deletes itself through the event loop
    bool reopenDialog() const
    {
        QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog.isNull() || mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s);
        if (auto* pDialog = dialog(); pDialog && pDialog->isVisible()) {
            return true;
        }
        if (!QTest::qWaitFor(
                    []() {
                        return mudlet::self()->mpConnectionDialog.isNull();
                    },
                    5s)) {
            return false;
        }
        mudlet::self()->slot_showConnectionDialog();
        return QTest::qWaitFor(
                []() {
                    return !mudlet::self()->mpConnectionDialog.isNull() && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s);
    }

    // slot_itemClicked() ignores a repeat of the profile it saw less than 100ms ago
    bool selectProfile(const QString& name) const
    {
        QTest::qWait(300ms);
        auto* pDialog = dialog();
        if (!pDialog) {
            return false;
        }
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, name, dlgConnectionProfiles::csmNameRole);
        if (items.isEmpty()) {
            return false;
        }
        pDialog->listWidget_profiles->setCurrentItem(items.first());
        pDialog->slot_itemClicked(items.first());
        return pDialog->profile_name_entry->text() == name;
    }

    Host* openOfflineThroughTheDialog(const QString& name) const
    {
        dialog()->slot_load();
        return HostManager::self()->getHost(name);
    }

    bool closeProfile(const QString& name) const
    {
        Host* pHost = HostManager::self()->getHost(name);
        if (!pHost) {
            return true;
        }
        // saves without asking
        pHost->mFORCE_SAVE_ON_EXIT = true;
        mudlet::self()->slot_closeProfileByName(name);
        return QTest::qWaitFor(
                [name]() {
                    return !HostManager::self()->getHost(name);
                },
                15s);
    }

    bool applySecureConnection(Host* pHost, const bool secure) const
    {
        dlgProfilePreferences preferences(mudlet::self(), pHost);
        preferences.show();
        if (!QTest::qWaitForWindowExposed(&preferences)) {
            return false;
        }
        QSignalSpy applySpy(&preferences, &dlgProfilePreferences::signal_preferencesSaved);
        preferences.groupBox_ssl->setChecked(secure);
        return TestSettings::waitForApply(applySpy);
    }

    static QString consoleText(Host* pHost) { return pHost->mainConsoleView()->buffer.lineBuffer.join(QChar::Space).simplified(); }

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
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        // keeps the first-run invitation, which hides the games list, out of the way
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        MudletApp::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        for (const auto& name : {mProfile, mUnwritableProfile}) {
            QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, name)));
            QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
            QVERIFY(MudletApp::writeProfileData(name, qsl("port"), qsl("1")).first);
            QVERIFY(MudletApp::writeProfileData(name, qsl("ssl_tsl"), QString::number(Qt::Unchecked)).first);
        }
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mTlsGame)));

        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    // The dialog is not modal, so it can sit open on the profile while the
    // setting is changed in Preferences, and be used to connect again after the
    // profile is closed - with the checkbox it read before the change
    void test_aChangeMadeWhileTheDialogIsOpenIsNotUndoneByIt()
    {
        QVERIFY(reopenDialog());
        QVERIFY(selectProfile(mProfile));
        Host* pHost = openOfflineThroughTheDialog(mProfile);
        QVERIFY2(pHost, "the test profile did not open");

        QVERIFY(reopenDialog());
        QVERIFY(selectProfile(mProfile));
        QVERIFY2(!dialog()->port_ssl_tsl->isChecked(), "the dialog already showed a secure connection, so this proves nothing");
        const QPointer<dlgConnectionProfiles> pDialogInUse = dialog();

        QVERIFY2(applySecureConnection(pHost, true), "turning on the secure connection never wrote the settings back");
        QVERIFY(closeProfile(mProfile));
        QVERIFY2(reopenDialog() && dialog() == pDialogInUse, "a different dialog came up, which reads the setting afresh and would hide the problem");

        pHost = openOfflineThroughTheDialog(mProfile);
        QVERIFY2(pHost, "the test profile did not open again");
        QVERIFY2(pHost->mSslTsl, "connecting from the dialog that was open turned the secure connection back off");
        QVERIFY(closeProfile(mProfile));
    }

    // With no saved port the dialog fills in the game's own details, and it
    // took the game's secure default along with them over the saved choice
    void test_aSavedChoiceOutranksTheGamesDefault()
    {
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mTlsGame)));
        QFile::remove(MudletApp::getMudletPath(enums::profileDataItemPath, mTlsGame, qsl("port")));
        QVERIFY(MudletApp::writeProfileData(mTlsGame, qsl("ssl_tsl"), QString::number(Qt::Unchecked)).first);

        QVERIFY(reopenDialog());
        QVERIFY(selectProfile(mTlsGame));
        QVERIFY2(!dialog()->port_ssl_tsl->isChecked(), "the game's default replaced the saved choice of an insecure connection");
        QCOMPARE(MudletApp::readProfileData(mTlsGame, qsl("ssl_tsl")).toInt(), static_cast<int>(Qt::Unchecked));
    }

    void test_aGameWithNoSavedChoiceStillGetsItsSecureDefault()
    {
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mTlsGame)));
        QFile::remove(MudletApp::getMudletPath(enums::profileDataItemPath, mTlsGame, qsl("port")));
        QFile::remove(MudletApp::getMudletPath(enums::profileDataItemPath, mTlsGame, qsl("ssl_tsl")));

        QVERIFY(reopenDialog());
        QVERIFY(selectProfile(mTlsGame));
        QVERIFY2(dialog()->port_ssl_tsl->isChecked(), "a game that defaults to a secure connection was offered an insecure one");
    }

    // The choice still applies to this session, but the dialog would undo it
    // on the next connect, which the player has to be told about
    void test_aChoiceTheDialogCannotSeeIsReported()
    {
        QVERIFY(reopenDialog());
        QVERIFY(selectProfile(mUnwritableProfile));
        Host* pHost = openOfflineThroughTheDialog(mUnwritableProfile);
        QVERIFY2(pHost, "the test profile did not open");
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");

        // a directory in the way stops the write even for root, who ignores permissions
        const QString file = MudletApp::getMudletPath(enums::profileDataItemPath, mUnwritableProfile, qsl("ssl_tsl"));
        QVERIFY(QFile::remove(file));
        QVERIFY(QDir().mkpath(qsl("%1/blocker").arg(file)));
        auto restore = qScopeGuard([file]() {
            QDir(file).removeRecursively();
        });

        QVERIFY2(applySecureConnection(pHost, true), "turning on the secure connection never wrote the settings back");
        QVERIFY(pHost->mSslTsl);
        const QString shown = consoleText(pHost);
        QVERIFY2(shown.contains(qsl("[ WARN ]")), qPrintable(qsl("the failed save was not reported; the console holds: %1").arg(shown)));
        QVERIFY(closeProfile(mUnwritableProfile));
    }
};

#include "ConnectionDialogSecureSettingTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogSecureSettingTest)
