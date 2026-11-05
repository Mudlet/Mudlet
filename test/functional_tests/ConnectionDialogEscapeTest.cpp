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
 * Escape closes the connection dialog from the games list too, except that
 * while a type-to-search there is running it clears that search first.
 *
 * Run with: ctest -R ConnectionDialogEscapeTest -V
 */

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QListWidget>
#include <QWindow>

#include "GroupedTest.h"
#include "ProfileTestHelper.h"

using namespace std::chrono_literals;

class ConnectionDialogEscapeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    const QString mFirstProfile = qsl("Aardvark-ConnDialogEscape");
    const QString mSecondProfile = qsl("Zebra-ConnDialogEscape");

    // a saved profile keeps the first-launch tutorial invitation - which hides
    // the games list altogether - out of the way
    bool makeProfileFolder(const QString& name) const { return QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), name)); }

    dlgConnectionProfiles* openDialog() const
    {
        mudlet::self()->slot_showConnectionDialog();
        // the dialog is only shown from a queued lambda, so the pointer turning
        // up is not enough
        if (!QTest::qWaitFor(
                    []() {
                        return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                    },
                    5s)) {
            return nullptr;
        }
        return mudlet::self()->mpConnectionDialog.data();
    }

    static bool focusOn(QWidget* widget)
    {
        widget->window()->activateWindow();
        widget->setFocus(Qt::OtherFocusReason);
        return TestProfile::waitForFocus(widget, 5s);
    }

    // to the window rather than to the dialog, so the key is routed to whichever
    // widget holds the focus and passes through the application event filter
    static void pressEscape(dlgConnectionProfiles* dialog) { QTest::keyClick(dialog->windowHandle(), Qt::Key_Escape); }

    static bool waitForClosed()
    {
        return QTest::qWaitFor(
                []() {
                    return !mudlet::self()->mpConnectionDialog || !mudlet::self()->mpConnectionDialog->isVisible();
                },
                2s);
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
        QVERIFY(makeProfileFolder(mFirstProfile));
        QVERIFY(makeProfileFolder(mSecondProfile));
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void cleanup()
    {
        if (mudlet::self()->mpConnectionDialog) {
            mudlet::self()->mpConnectionDialog->close();
            QVERIFY(QTest::qWaitFor(
                    []() {
                        return !mudlet::self()->mpConnectionDialog;
                    },
                    2s));
        }
    }

    void test_escapeOnTheGamesListClosesTheDialog()
    {
        auto* dialog = openDialog();
        QVERIFY2(dialog, "The connection dialog never came up");
        QVERIFY2(dialog->listWidget_profiles->isVisible(), "The games list is not on screen, so this is not the dialog the test is about");
        QVERIFY2(focusOn(dialog->listWidget_profiles), "Could not put the focus on the games list");

        QSignalSpy rejected(dialog, &QDialog::rejected);
        pressEscape(dialog);

        QVERIFY2(waitForClosed(), "Escape on the games list left the dialog open");
        QCOMPARE(rejected.count(), 1);
    }

    void test_escapeClearsASearchBeforeClosingTheDialog()
    {
        auto* dialog = openDialog();
        QVERIFY2(dialog, "The connection dialog never came up");
        auto* list = dialog->listWidget_profiles;
        QVERIFY2(list->isVisible(), "The games list is not on screen, so this is not the dialog the test is about");
        QVERIFY2(focusOn(list), "Could not put the focus on the games list");

        // the search lapses after a second, and a slow runner must not see a
        // lapsed search close the dialog where this test expects a clear
        dialog->mSearchTextTimer.setInterval(1h);
        QSignalSpy rejected(dialog, &QDialog::rejected);

        // a letter only the second profile starts with, so the first gets greyed out
        QTest::keyClick(dialog->windowHandle(), static_cast<Qt::Key>(mSecondProfile.at(0).toUpper().unicode()));
        const auto firstItems = dialog->findData(*list, mFirstProfile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!firstItems.isEmpty(), "The first test profile is missing from the games list");
        QVERIFY2(!(firstItems.first()->flags() & Qt::ItemIsEnabled), "Typing did not start a search, so this test cannot tell a clear from a close");

        pressEscape(dialog);
        QTest::qWait(50ms);
        QVERIFY2(dialog->isVisible(), "The first Escape closed the dialog instead of clearing the search");
        QCOMPARE(rejected.count(), 0);
        QVERIFY2(firstItems.first()->flags() & Qt::ItemIsEnabled, "Escape did not clear the search");

        // the key held down after clearing the search, rather than pressed again
        QKeyEvent autoRepeat(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier, QString(), true);
        QApplication::sendEvent(list, &autoRepeat);
        QTest::qWait(50ms);
        QVERIFY2(dialog->isVisible(), "Holding Escape down to clear a search went on to close the dialog");
        QCOMPARE(rejected.count(), 0);

        pressEscape(dialog);
        QVERIFY2(waitForClosed(), "A second Escape, with no search left to clear, did not close the dialog");
        QCOMPARE(rejected.count(), 1);
    }

    void test_escapeClosesTheDialogOnceASearchHasLapsed()
    {
        auto* dialog = openDialog();
        QVERIFY2(dialog, "The connection dialog never came up");
        auto* list = dialog->listWidget_profiles;
        QVERIFY2(list->isVisible(), "The games list is not on screen, so this is not the dialog the test is about");
        QVERIFY2(focusOn(list), "Could not put the focus on the games list");

        QSignalSpy rejected(dialog, &QDialog::rejected);
        QTest::keyClick(dialog->windowHandle(), static_cast<Qt::Key>(mSecondProfile.at(0).toUpper().unicode()));

        // let the search lapse now rather than after its usual second
        dialog->mSearchTextTimer.start(0ms);
        QVERIFY(QTest::qWaitFor(
                [dialog]() {
                    return !dialog->mSearchTextTimer.isActive();
                },
                2s));

        pressEscape(dialog);
        QVERIFY2(waitForClosed(), "Escape after a search had lapsed only cleared it rather than closing the dialog");
        QCOMPARE(rejected.count(), 1);
    }
};

MUDLET_GROUPED_TEST_MAIN(ConnectionDialogEscapeTest)
#include "ConnectionDialogEscapeTest.moc"
