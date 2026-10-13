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
#include <QListWidget>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>

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

        // selecting the profile wrote its empty description
        const QString descriptionPath = MudletApp::getMudletPath(enums::profileDataItemPath, profile, qsl("description"));
        QFile::remove(descriptionPath);
        QVERIFY(QDir().mkpath(descriptionPath));
        expectWriteWarning(qsl("description"));
        dialog->mud_description_textedit->setPlainText(qsl("not saved"));

        QVERIFY2(dialog->notificationArea->isVisible(), "the dialog says nothing about a change it could not save");
        QVERIFY2(dialog->notificationAreaMessageBox->text().contains(profile), qPrintable(dialog->notificationAreaMessageBox->text()));
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
};

#include "ProfileDataWriteTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileDataWriteTest)
