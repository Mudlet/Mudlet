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
 * Where the keyboard focus lands when the connection dialog opens. It has to be
 * the games list: typing there picks a game, whereas the profile name field
 * silently renames whichever profile is selected - see issue #10307.
 *
 * Run with: ctest -R ConnectionDialogFocusTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLineEdit>
#include <QListWidget>
#include <QWindow>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogFocusTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // two of them, so typing has somewhere to move the selection to
    const QString mFirstProfile = qsl("Aardvark-ConnDialogFocus");
    const QString mSecondProfile = qsl("Zebra-ConnDialogFocus");

    // a saved profile keeps the first-launch tutorial invitation - which hides
    // the games list altogether - out of the way
    bool makeProfileFolder(const QString& name) const { return QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), name)); }

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

    void test_gamesListHasTheOpeningFocus()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");
        QVERIFY2(dialog->listWidget_profiles->isVisible(), "The games list is not on screen, so this is not the dialog the test is about");

        // the focus is only handed out once the dialog is activated, so wait for
        // it to arrive rather than reading it the moment the dialog is up
        QTest::qWaitFor(
                [dialog]() {
                    return QApplication::focusWidget() == dialog->listWidget_profiles;
                },
                5000);

        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(dialog->listWidget_profiles));
    }

    void test_typingPicksAGameRatherThanRenamingAProfile()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");
        auto* list = dialog->listWidget_profiles;

        // select the first profile outright rather than relying on whatever the
        // list starts on, so the only thing this measures is where the keystroke
        // below lands
        const auto items = dialog->findData(*list, mFirstProfile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The first test profile is missing from the games list");
        list->setCurrentItem(items.first());

        // to the window rather than to the dialog, so the key is routed to
        // whichever widget holds the focus - events sent to the dialog
        // propagate up from it and never reach its focused child
        QTest::keyClick(dialog->windowHandle(), static_cast<Qt::Key>(mSecondProfile.at(0).toUpper().unicode()));
        QTest::qWait(100ms);

        QVERIFY2(list->currentItem(), "Typing cleared the games list selection");
        QCOMPARE(list->currentItem()->data(dlgConnectionProfiles::csmNameRole).toString(), mSecondProfile);
        QCOMPARE(dialog->profile_name_entry->text(), mSecondProfile);
    }
};

MUDLET_GROUPED_TEST_MAIN(ConnectionDialogFocusTest)
#include "ConnectionDialogFocusTest.moc"
