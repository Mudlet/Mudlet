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

// A spec cannot fill a disk, so a file size limit on this process stands in for one:
// past it write() fails with EFBIG, as it fails with ENOSPC on a full disk.

#include <QtTest/QtTest>

#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>

#include "CredentialManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#if defined(Q_OS_UNIX)
#include <algorithm>
#include <cerrno>
#include <csignal>
#include <sys/resource.h>
#endif

#include "GroupedTest.h"
#include "PortableModeTestHelper.h"

using namespace std::chrono_literals;

class ProfileDataWriteTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfile = qsl("ProfileDataWriteProfile");
    const QString mItem = qsl("description");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    bool mMudletStarted = false;

    QStringList filesForTheItem() const { return QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfile)).entryList({qsl("%1*").arg(mItem)}, QDir::Files | QDir::Hidden); }

    // Also keeps the warning off stderr, which under the limit would lose it if stderr is a file past the limit
    void expectWriteWarning(const QString& item) { QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl(R"(^MudletApp::writeProfileData\(\.\.\.\) ERROR - .*item: "%1")").arg(item))); }

#if defined(Q_OS_UNIX)
    void writeUnderFileSizeLimit(const rlim_t limit, const QString& what, QPair<bool, QString>& result)
    {
        rlimit saved{};
        QCOMPARE(::getrlimit(RLIMIT_FSIZE, &saved), 0);
        rlimit limited = saved;
        limited.rlim_cur = std::min(limit, saved.rlim_max);
        // Past the limit the kernel also raises SIGXFSZ, whose default action ends the process
        const auto previousHandler = std::signal(SIGXFSZ, SIG_IGN);
        const bool applied = ::setrlimit(RLIMIT_FSIZE, &limited) == 0;
        if (applied) {
            result = MudletApp::writeProfileData(mProfile, mItem, what);
        }
        const bool restored = ::setrlimit(RLIMIT_FSIZE, &saved) == 0;
        std::signal(SIGXFSZ, previousHandler);
        QVERIFY2(applied, "the file size limit could not be set");
        QVERIFY2(restored, "the file size limit could not be put back");
    }
#endif

    void startMudlet()
    {
        if (mMudletStarted) {
            return;
        }
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mMudletStarted = true;
    }

    // The dialog lists the profiles there were when it opened, so a test that makes its own reopens it
    dlgConnectionProfiles* reopenConnectionDialog()
    {
        startMudlet();
        if (mudlet::self()->mpConnectionDialog) {
            mudlet::self()->mpConnectionDialog->close();
            if (!QTest::qWaitFor(
                        []() {
                            return !mudlet::self()->mpConnectionDialog;
                        },
                        5s)) {
                return nullptr;
            }
        }
        mudlet::self()->startAutoLogin({});
        if (!QTest::qWaitFor(
                    []() {
                        return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                    },
                    5s)) {
            return nullptr;
        }
        return mudlet::self()->mpConnectionDialog.data();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config root cannot be redirected away from the real one");
        }

        QVERIFY(mConfigDir.isValid());
        // An XDG_CONFIG_HOME/mudlet without profiles/ loses to a ~/.config/mudlet that has some, which
        // would put this test's writes among the real profiles
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        const QString profileHome = MudletApp::getMudletPath(enums::profileHomePath, mProfile);
        QVERIFY2(profileHome.startsWith(qsl("%1/mudlet/").arg(mConfigDir.path())), qPrintable(qsl("the profile would be written under %1").arg(profileHome)));
    }

    // The profile is removed with mConfigDir, never through a resolved path that might be a real one
    void cleanupTestCase()
    {
        if (mMudletStarted) {
            delete mudlet::self();
        }
        mSavedXdgConfigHome.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
    }

    // Some of the data lands, so the file itself records no error
    void test_aWriteThatCannotLandIsReportedAndKeepsTheOldValue()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to make a write fail");
#else
        QVERIFY(MudletApp::writeProfileData(mProfile, mItem, qsl("kept")).first);

        // 2 MiB once written as UTF-16, well past the limit
        const QString tooBig(1024 * 1024, QLatin1Char('x'));
        QPair<bool, QString> result{true, QString()};
        expectWriteWarning(mItem);
        writeUnderFileSizeLimit(64 * 1024, tooBig, result);
        if (QTest::currentTestFailed()) {
            return;
        }

        QVERIFY2(!result.first, "a write that could not land was reported as a success");
        QVERIFY2(result.second.contains(mItem), qPrintable(qsl("the reason does not say what was not written: %1").arg(result.second)));
        QCOMPARE(MudletApp::readProfileData(mProfile, mItem), qsl("kept"));
        // The QSaveFile's own temporary file goes with the failure
        QCOMPARE(filesForTheItem(), QStringList{mItem});
#endif
    }

    // Nothing lands, so the error is on the file, and commit()'s close() would clear it
    void test_aWriteThatLandsNothingIsReportedWithTheSystemsReason()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to make a write fail");
#else
        QVERIFY(MudletApp::writeProfileData(mProfile, mItem, qsl("kept")).first);

        QPair<bool, QString> result{true, QString()};
        expectWriteWarning(mItem);
        writeUnderFileSizeLimit(0, qsl("lost"), result);
        if (QTest::currentTestFailed()) {
            return;
        }

        QVERIFY2(!result.first, "a write that landed nothing was reported as a success");
        QCOMPARE(result.second, qt_error_string(EFBIG));
        QCOMPARE(MudletApp::readProfileData(mProfile, mItem), qsl("kept"));
        QCOMPARE(filesForTheItem(), QStringList{mItem});
#endif
    }

    void test_aFileThatCannotBeOpenedIsReportedAndLogged()
    {
        const QString item = qsl("port");
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileDataItemPath, mProfile, item)));

        expectWriteWarning(item);
        const auto [ok, reason] = MudletApp::writeProfileData(mProfile, item, qsl("23"));

        QVERIFY2(!ok, "writing over a directory was reported as a success");
        QVERIFY2(!reason.isEmpty(), "a failed open gave no reason");
        QVERIFY(QDir().rmdir(MudletApp::getMudletPath(enums::profileDataItemPath, mProfile, item)));
    }

    void test_aDetailTheConnectionDialogCannotSaveIsShownThere()
    {
        const QString profile = qsl("ProfileDataWriteDialog");
        QVERIFY(MudletApp::writeProfileData(profile, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(profile, qsl("port"), qsl("23")).first);
        startMudlet();
        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        const auto items = dialog->findData(*dialog->listWidget_profiles, profile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "the test profile is missing from the games list");
        dialog->listWidget_profiles->setCurrentItem(items.first());
        QVERIFY2(dialog->notificationAreaMessageBox->text().isEmpty(), "the dialog already shows a notice, so this proves nothing");

        const QString descriptionPath = MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("description"));
        QFile::remove(descriptionPath);
        QVERIFY(QDir().mkpath(descriptionPath));
        expectWriteWarning(qsl("description"));
        dialog->mud_description_textedit->setPlainText(qsl("not saved"));

        QVERIFY2(dialog->notificationArea->isVisible(), "the dialog says nothing about a change it could not save");
        QVERIFY2(dialog->notificationAreaMessageBox->text().contains(profile), qPrintable(dialog->notificationAreaMessageBox->text()));
        QVERIFY(QDir().rmdir(descriptionPath));

        dialog->mud_description_textedit->setPlainText(qsl("saved"));
        QCOMPARE(MudletApp::readProfileData(profile, qsl("description")), qsl("saved"));
        QVERIFY2(!dialog->notificationArea->isVisible(), "the notice stays up after the change was saved");
    }

    void test_aLoginOrPasswordTheConnectionDialogCannotSaveIsShownThere()
    {
        const QString profile = qsl("ProfileDataWriteCredentials");
        QVERIFY(MudletApp::writeProfileData(profile, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(profile, qsl("port"), qsl("23")).first);
        auto* dialog = reopenConnectionDialog();
        QVERIFY(dialog);
        mudlet::self()->setStorePasswordsSecurely(false);
        const auto items = dialog->findData(*dialog->listWidget_profiles, profile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "the test profile is missing from the games list");
        dialog->listWidget_profiles->setCurrentItem(items.first());
        QVERIFY2(!dialog->notificationArea->isVisible(), "the dialog already shows a notice, so this proves nothing");
        // Selecting loads the password on a zero timer, which would put the field back after it is typed in
        QCoreApplication::processEvents();

        const QString passwordPath = MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("password"));
        QVERIFY(QDir().mkpath(passwordPath));
        expectWriteWarning(qsl("password"));
        dialog->character_password_entry->setText(qsl("not saved"));
        QTRY_VERIFY2(dialog->notificationArea->isVisible(), "the dialog says nothing about a password it could not save");
        QVERIFY2(dialog->notificationAreaMessageBox->text().contains(profile), qPrintable(dialog->notificationAreaMessageBox->text()));
        QVERIFY(QDir().rmdir(passwordPath));
        dialog->character_password_entry->setText(qsl("saved"));
        QTRY_COMPARE(MudletApp::readProfileData(profile, qsl("password")), qsl("saved"));
        QVERIFY2(!dialog->notificationArea->isVisible(), "the notice stays up after the password was saved");

        const QString loginPath = MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("login"));
        QVERIFY(QDir().mkpath(loginPath));
        expectWriteWarning(qsl("login"));
        QTest::keyClicks(dialog->login_entry, qsl("x"));
        QVERIFY2(dialog->notificationArea->isVisible(), "the dialog says nothing about a character name it could not save");
        QVERIFY(QDir().rmdir(loginPath));
    }

    void test_selectingAProfileThatCannotBeWrittenRaisesNoNotice()
    {
        const QString profile = qsl("ProfileDataWriteUnwritable");
        QVERIFY(MudletApp::writeProfileData(profile, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(profile, qsl("port"), qsl("23")).first);
        const QString descriptionPath = MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("description"));
        QVERIFY(QDir().mkpath(descriptionPath));
        auto* dialog = reopenConnectionDialog();
        QVERIFY(dialog);
        const auto items = dialog->findData(*dialog->listWidget_profiles, profile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "the test profile is missing from the games list");

        QTest::failOnWarning(QRegularExpression(qsl("writeProfileData")));
        dialog->listWidget_profiles->setCurrentItem(items.first());

        QVERIFY2(!dialog->notificationArea->isVisible(), qPrintable(dialog->notificationAreaMessageBox->text()));
        QVERIFY(QDir().rmdir(descriptionPath));
    }

    // A game on the list that has never been played has no folder, and a detail typed for it must not make one
    void test_aDetailForAGameNeverPlayedIsQuietlyDropped()
    {
        startMudlet();
        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QListWidgetItem* unplayed = nullptr;
        for (int i = 0; i < dialog->listWidget_profiles->count() && !unplayed; ++i) {
            const QString name = dialog->listWidget_profiles->item(i)->data(dlgConnectionProfiles::csmNameRole).toString();
            if (!QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).exists()) {
                unplayed = dialog->listWidget_profiles->item(i);
            }
        }
        if (!unplayed) {
            QSKIP("every game on the list has a folder already");
        }
        const QString game = unplayed->data(dlgConnectionProfiles::csmNameRole).toString();
        dialog->listWidget_profiles->setCurrentItem(unplayed);
        const QString noticeBefore = dialog->notificationAreaMessageBox->text();

        QTest::failOnWarning(QRegularExpression(qsl("writeProfileData")));
        dialog->mud_description_textedit->setPlainText(qsl("typed for a game never played"));

        QVERIFY2(!QDir(MudletApp::getMudletPath(enums::profileHomePath, game)).exists(), qPrintable(qsl("a folder was made for %1, which has never been played").arg(game)));
        QCOMPARE(dialog->notificationAreaMessageBox->text(), noticeBefore);
    }

    void test_aDetailACopyCannotSaveIsShownAfterTheCopy()
    {
        startMudlet();
        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QListWidgetItem* unplayed = nullptr;
        for (int i = 0; i < dialog->listWidget_profiles->count() && !unplayed; ++i) {
            const QString name = dialog->listWidget_profiles->item(i)->data(dlgConnectionProfiles::csmNameRole).toString();
            if (!name.back().isDigit() && !QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).exists() && !QDir(MudletApp::getMudletPath(enums::profileHomePath, name + qsl("1"))).exists()) {
                unplayed = dialog->listWidget_profiles->item(i);
            }
        }
        if (!unplayed) {
            QSKIP("every game on the list has a folder already");
        }
        const QString game = unplayed->data(dlgConnectionProfiles::csmNameRole).toString();
        const QString copy = game + qsl("1");
        dialog->listWidget_profiles->setCurrentItem(unplayed);
        dialog->mud_description_textedit->setPlainText(qsl("copied with the game"));
        // Copying a game never played writes each detail of the copy itself
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileDataItemPath, copy, qsl("description"))));

        expectWriteWarning(qsl("description"));
        dialog->slot_copyProfile();

        QCOMPARE(MudletApp::readProfileData(copy, qsl("url")), dialog->host_name_entry->text());
        QVERIFY2(dialog->notificationArea->isVisible(), "the dialog says nothing about a detail of the copy it could not save");
        QVERIFY2(dialog->notificationAreaMessageBox->text().contains(copy), qPrintable(dialog->notificationAreaMessageBox->text()));
    }

    // Moving a password out of secure storage must not delete it there when it could not be saved here. Last, as the
    // keychain lookups it starts can answer during a later test, and the connection dialog reselects its profile then
    void test_aPasswordThatCannotBeMigratedStaysInSecureStorage()
    {
        const QString profile = qsl("ProfileDataWriteMigrate");
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("password"))));
        startMudlet();
        QVERIFY(CredentialManager::storeCredential(profile, qsl("character"), qsl("secret")));

        expectWriteWarning(qsl("password"));
        // Whether secure storage keeps the password is settled before this returns; the
        // legacy keychain lookups it also starts can outlast any wait on macOS runners
        QVERIFY(mudlet::self()->migratePasswordsToProfileStorage());

        QCOMPARE(CredentialManager::retrieveCredential(profile, qsl("character")), qsl("secret"));
        QVERIFY2(mudlet::self()->passwordsNotMigratedToProfiles().contains(profile), "the migration reports every password as moved");
        QVERIFY(QDir().rmdir(MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("password"))));
    }

    void test_aLegacyKeychainPasswordThatCannotBeMigratedIsReported()
    {
        const QString profile = qsl("ProfileDataWriteLegacyMigrate");
        const QString passwordPath = MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("password"));
        QVERIFY(QDir().mkpath(passwordPath));
        startMudlet();
        // A job that never ran reads as a successful lookup, so the slot goes on to save what it holds
        auto* job = new QKeychain::ReadPasswordJob(qsl("Mudlet profile"));
        job->setAutoDelete(false);
        job->setProperty("profile", profile);

        expectWriteWarning(qsl("password"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("could not migrate the password for profile \"%1\", so it stays in the legacy keychain storage").arg(profile)));
        QVERIFY(QMetaObject::invokeMethod(mudlet::self(), "slot_passwordMigratedToPortableStorage", Qt::DirectConnection, Q_ARG(QKeychain::Job*, job)));

        QVERIFY2(mudlet::self()->passwordsNotMigratedToProfiles().contains(profile), "the migration reports a password it could not save as moved");
        QVERIFY(QDir().rmdir(passwordPath));
    }
};

#include "ProfileDataWriteTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileDataWriteTest)
