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
 * Where the keyboard focus lands when the first-launch tutorial invitation is
 * skipped. Restoring the games list hides the widgets the invitation put up,
 * which scatters the focus on down the chain to the profile name field - where
 * typing a game's name renames the selected profile. See issue #10307.
 *
 * Run with: ctest -R TutorialInvitationFocusTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QListWidget>
#include <QPushButton>

#include "GroupedTest.h"

class TutorialInvitationFocusTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

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
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mudlet::self()->startAutoLogin({});
        // the dialog is only shown from a queued lambda, so the pointer turning
        // up is not enough
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

    void test_skippingTheInvitationFocusesTheGamesList()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");

        auto* skipButton = dialog->findChild<QPushButton*>(qsl("skipToGamesButton"));
        QVERIFY2(skipButton, "The first-launch invitation has no skip button any more");
        QVERIFY2(skipButton->isVisible(), "This is not a first-launch dialog - the skip button is not shown");
        skipButton->click();
        // the focus only reaches the application once the dialog is activated,
        // which can happen either side of the click
        QTest::qWaitFor(
                [dialog]() {
                    return QApplication::focusWidget() == dialog->listWidget_profiles;
                },
                5000);

        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(dialog->listWidget_profiles));
    }
};

MUDLET_GROUPED_TEST_MAIN(TutorialInvitationFocusTest)
#include "TutorialInvitationFocusTest.moc"
