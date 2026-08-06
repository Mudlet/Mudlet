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
 * Three ways the connection dialog dereferenced a pointer that was not there,
 * all driven through the real dialog against an isolated config directory:
 *
 * - Right-clicking the games list with no profile selected. The "My games" tab
 *   of a fresh install is that state: nothing on disk to list, so fillout_form()
 *   selects nothing, and slot_profileContextMenu() read currentItem() anyway.
 * - Copying a profile while the list is rebuilt underneath the copy. The copy
 *   runs on a thread pool and reports back through the event loop, so anything
 *   that calls fillout_form() in between - switching the games tab is one click
 *   away - destroyed the QListWidgetItem the completion handler had kept.
 * - Quitting before the queued lambda in mudlet::slot_showConnectionDialog()
 *   has had its turn. mudlet::closeEvent() closes the dialog and clears the
 *   QPointer to it, and the lambda then showed a dialog that was gone.
 *
 * Each of these is a crash rather than a wrong answer, so the assertions are
 * mostly "the run got this far"; the extra checks pin the behaviour that
 * replaced the crash, so a fix that merely returns early is not enough.
 *
 * Run with: ctest -R ConnectionDialogCrashTest -V
 */

#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QAbstractScrollArea>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPushButton>
#include <QTabBar>
#include <chrono>

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

static void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

class ConnectionDialogCrashTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // Deliberately not created in initTestCase(): the first test needs the
    // genuine fresh-install state, with nothing at all under profiles/
    const QString mProfileName = qsl("ConnDialogCrash-Test");
    // what copyProfileWidget() names the copy of a name not ending in a digit
    const QString mCopyName = qsl("ConnDialogCrash-Test1");
    const QString mQuietProfileName = qsl("ConnDialogCrash-Quiet");
    const QString mQuietCopyName = qsl("ConnDialogCrash-Quiet1");

    static constexpr int scmMyGamesTab = 0;
    static constexpr int scmAllGamesTab = 1;
    // a role of this test's own, well clear of dlgConnectionProfiles::csmNameRole
    static constexpr int scmTestMarkerRole = Qt::UserRole + 99;
    // what makeProfileFolder() writes, so a copy can be checked for carrying it
    const QString mProfileUrl = qsl("mudlet.org");
    const QString mProfilePort = qsl("23");

    // setupConfig() consults portable.txt before the XDG logic; skip rather
    // than run against an unexpected config dir (see ConfigDirOverrideTest).
    bool portableMarkerPresent() const
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // The tab bar splitting "My games" from "All games" is a private member of
    // the dialog. It is not the dialog's only QTabBar - QTabWidget builds one
    // for itself too - so ask for it by name.
    QTabBar* gamesTabBar(dlgConnectionProfiles* dialog) const { return dialog->findChild<QTabBar*>(qsl("gamesTabBar")); }

    // What the last armMenuCloser() run saw. Members rather than locals of the
    // calling test method: when no menu appears the timer is still armed when
    // that method returns, and writing into a stack frame that has gone is the
    // very bug this file exists to catch.
    QStringList mMenuActionTexts;
    bool mSawMenu = false;
    QString mUnexpectedPopup;

    // The context menu runs its own event loop in menu.exec(), so whatever it
    // puts on screen has to be inspected and dismissed from inside that loop.
    // Callers stop the returned timer, which is what ends it when no menu turns
    // up - the case the crash tests want.
    QTimer* armMenuCloser()
    {
        mMenuActionTexts.clear();
        mSawMenu = false;
        mUnexpectedPopup.clear();
        auto* closer = new QTimer(this);
        closer->setInterval(20);
        connect(closer, &QTimer::timeout, this, [this, closer]() {
            auto* popup = QApplication::activePopupWidget();
            if (!popup) {
                return;
            }
            auto* menu = qobject_cast<QMenu*>(popup);
            if (!menu) {
                // something else has the popup and menu.exec() is waiting on
                // it, so close it anyway rather than let the run time out
                mUnexpectedPopup = QString::fromLatin1(popup->metaObject()->className());
                popup->close();
                closer->stop();
                return;
            }
            mSawMenu = true;
            const auto actions = menu->actions();
            for (const auto* action : actions) {
                mMenuActionTexts << action->text();
            }
            menu->close();
            closer->stop();
        });
        closer->start();
        return closer;
    }

    // menu.exec() has returned by now either way, so the closer has done its job
    void disarmMenuCloser(QTimer* closer)
    {
        closer->stop();
        closer->deleteLater();
    }

    // so that a popup from somewhere else shows up in the failure rather than
    // just looking like "no menu appeared"
    QString menuOutcome() const { return mUnexpectedPopup.isEmpty() ? QString() : qsl(" (a %1 took the popup instead)").arg(mUnexpectedPopup); }

    // What a right-click on the list does. The event has to go to the viewport,
    // the way the window system delivers it: QAbstractScrollArea ignores a
    // mouse-reason context menu sent to the scroll area itself, and it is its
    // viewportEvent() that turns the viewport's event into the list's own
    // customContextMenuRequested.
    void rightClickBelowTheLastItem(QAbstractScrollArea* view) const
    {
        auto* viewport = view->viewport();
        const QPoint pos(viewport->width() / 2, viewport->height() - 4);
        QContextMenuEvent event(QContextMenuEvent::Mouse, pos, viewport->mapToGlobal(pos));
        QApplication::sendEvent(viewport, &event);
    }

    // returns rather than QVERIFYs: a QVERIFY here would only leave this
    // helper, and the caller would carry on without the profile
    bool makeProfileFolder(const QString& name) const
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, name)) && mudlet::self()->writeProfileData(name, qsl("url"), mProfileUrl).first
               && mudlet::self()->writeProfileData(name, qsl("port"), mProfilePort).first;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        initializeQRCResources();

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet").arg(mXdgDir.path()))); // empty dir = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        // never touch the user's real profiles:
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mudlet::self()->startAutoLogin({});
        // slot_showConnectionDialog() only shows the dialog from a queued
        // lambda, so wait for that rather than for the pointer
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

    // C9: ~40 seconds into a fresh install - skip the tutorial invitation,
    // click "My games", right-click the list. There is no profile on disk, so
    // fillout_form() leaves nothing current and the menu had nothing to act on.
    void test_rightClickWithNoProfileSelected()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");

        auto* skipButton = dialog->findChild<QPushButton*>(qsl("skipToGamesButton"));
        QVERIFY2(skipButton, "The first-launch invitation has no skip button any more");
        QVERIFY2(skipButton->isVisible(), "This is not a first-launch dialog - the skip button is not shown");
        skipButton->click();
        QTest::qWait(100ms);

        auto* tabBar = gamesTabBar(dialog);
        QVERIFY2(tabBar, "The games list has no tab bar");
        tabBar->setCurrentIndex(scmMyGamesTab);
        QTest::qWait(100ms);

        // the state the crash needs. It is not the same as an empty list: a
        // debug build still offers the self-test entry here, and fillout_form()
        // makes nothing current because none of what it listed is on disk
        QVERIFY2(!dialog->listWidget_profiles->currentItem(),
                 qPrintable(
                         qsl("The 'My games' tab of a fresh install selected something (%1 items listed) - this test no longer covers the reported crash").arg(dialog->listWidget_profiles->count())));

        auto* closer = armMenuCloser();
        rightClickBelowTheLastItem(dialog->listWidget_profiles);
        disarmMenuCloser(closer);

        QVERIFY2(!mSawMenu, "A context menu was offered with no profile for it to act on");
        QVERIFY2(mUnexpectedPopup.isEmpty(), qPrintable(menuOutcome()));
        QVERIFY2(!QApplication::activePopupWidget(), "A popup was left on screen");
    }

    // The same right-click against a list with no items in it at all
    void test_rightClickOnAnEmptyList()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");

        dialog->listWidget_profiles->clear();
        QCOMPARE(dialog->listWidget_profiles->count(), 0);

        auto* closer = armMenuCloser();
        rightClickBelowTheLastItem(dialog->listWidget_profiles);
        disarmMenuCloser(closer);

        QVERIFY2(!mSawMenu, "A context menu was offered for an empty games list");
        QVERIFY2(mUnexpectedPopup.isEmpty(), qPrintable(menuOutcome()));

        // put the list back, rather than leaving the next test to notice
        dialog->fillout_form();
        QTest::qWait(100ms);
    }

    // The other half of the guard: with a profile selected the menu still has
    // to appear, so that "return early when nothing is current" cannot be
    // mistaken for "return early".
    void test_contextMenuStillOpensForASelectedProfile()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");

        auto* tabBar = gamesTabBar(dialog);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(scmAllGamesTab);
        QTest::qWait(100ms);

        QVERIFY2(dialog->listWidget_profiles->count() > 0, "The 'All games' tab lists nothing");
        dialog->listWidget_profiles->setCurrentRow(0);
        QVERIFY2(dialog->listWidget_profiles->currentItem(), "Could not select a profile to open the menu for");

        auto* closer = armMenuCloser();
        rightClickBelowTheLastItem(dialog->listWidget_profiles);
        disarmMenuCloser(closer);

        QVERIFY2(mSawMenu, qPrintable(qsl("No context menu appeared for a selected profile%1").arg(menuOutcome())));
        // "Set custom icon" and "Set custom color" for a profile without one
        QCOMPARE(mMenuActionTexts.size(), 2);
        QVERIFY2(!mMenuActionTexts.first().isEmpty(), "The menu offered a nameless action");
    }

    // C2/F10: the copy is asynchronous and the dialog stays usable while it
    // runs, so switching the games tab mid-copy rebuilds the list and destroys
    // the item the completion handler was holding on to.
    void test_copiedProfileSurvivesTheListBeingRebuilt()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");

        QVERIFY(makeProfileFolder(mProfileName));
        QDir(mudlet::getMudletPath(enums::profileHomePath, mCopyName)).removeRecursively();

        auto* tabBar = gamesTabBar(dialog);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(scmMyGamesTab);
        dialog->fillout_form();
        // slot_itemClicked() ignores a second selection of the same profile
        // within 100ms, and fillout_form() clears the form before it re-selects
        QTest::qWait(300ms);

        const auto items = dialog->findData(*dialog->listWidget_profiles, mProfileName, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The test profile is not listed in the dialog");
        dialog->listWidget_profiles->setCurrentItem(items.first());
        dialog->slot_itemClicked(items.first());
        QCOMPARE(dialog->profile_name_entry->text(), mProfileName);

        auto* copyAction = dialog->findChild<QAction*>(qsl("copyProfile"));
        QVERIFY2(copyAction, "The dialog has no Copy action any more");

        dialog->slot_copyProfile();
        QVERIFY2(!copyAction->isEnabled(), "The copy did not take the asynchronous path");
        // Nothing of the copy has completed: it runs on a thread pool and
        // reports back through the event loop, which has not run since. This is
        // the user clicking the other games tab while "Copying..." is up, and
        // it destroys every item in the list - including the copy's.
        tabBar->setCurrentIndex(scmAllGamesTab);

        // the completion handler runs in here, and used to read the freed item
        QVERIFY2(QTest::qWaitFor(
                         [copyAction]() {
                             return copyAction->isEnabled();
                         },
                         15000),
                 "The copy never completed");

        QVERIFY2(QDir(mudlet::getMudletPath(enums::profileHomePath, mCopyName)).exists(), "The copy has no folder on disk");
        QCOMPARE(dialog->readProfileData(mCopyName, qsl("url")), mProfileUrl);
        QCOMPARE(dialog->readProfileData(mCopyName, qsl("port")), mProfilePort);
        QVERIFY2(!dialog->findData(*dialog->listWidget_profiles, mCopyName, dlgConnectionProfiles::csmNameRole).isEmpty(), "The copy is not listed in the games list");
        // the point of the completion handler: the copy ends up selected.
        // The form fields are deliberately not asserted here - slot_itemClicked()
        // drops a second selection of the same profile within 100ms, and a copy
        // of an ordinary profile completes well inside that window, so an
        // undisturbed copy leaves Server address and Port blank. That is a
        // separate, pre-existing bug and not this PR's to fix.
        auto* pCurrentItem = dialog->listWidget_profiles->currentItem();
        QVERIFY2(pCurrentItem, "Nothing is selected after the copy finished");
        QCOMPARE(pCurrentItem->data(dlgConnectionProfiles::csmNameRole).toString(), mCopyName);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mCopyName)).removeRecursively();
        QDir(mudlet::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        dialog->fillout_form();
        QTest::qWait(100ms);
    }

    // The same copy with nothing disturbing the list, which is the other branch
    // of the completion handler: the item it made is still there and still
    // current, so it is used directly rather than looked up again.
    void test_copiedProfileIsSelectedWhenTheListIsLeftAlone()
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        QVERIFY2(dialog, "No connection dialog to test against");

        QVERIFY(makeProfileFolder(mQuietProfileName));
        QDir(mudlet::getMudletPath(enums::profileHomePath, mQuietCopyName)).removeRecursively();

        auto* tabBar = gamesTabBar(dialog);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(scmMyGamesTab);
        dialog->fillout_form();
        QTest::qWait(300ms);

        const auto items = dialog->findData(*dialog->listWidget_profiles, mQuietProfileName, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The test profile is not listed in the dialog");
        dialog->listWidget_profiles->setCurrentItem(items.first());
        dialog->slot_itemClicked(items.first());
        QCOMPARE(dialog->profile_name_entry->text(), mQuietProfileName);

        auto* copyAction = dialog->findChild<QAction*>(qsl("copyProfile"));
        QVERIFY(copyAction);
        dialog->slot_copyProfile();
        QVERIFY2(!copyAction->isEnabled(), "The copy did not take the asynchronous path");

        const auto created = dialog->findData(*dialog->listWidget_profiles, mQuietCopyName, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!created.isEmpty(), "The copy got no entry in the list");
        // marks the item slot_copyProfile() made. Checking for the mark
        // afterwards says whether that same item is still there without
        // holding on to a pointer that a rebuild would have freed.
        created.first()->setData(scmTestMarkerRole, true);

        QVERIFY2(QTest::qWaitFor(
                         [copyAction]() {
                             return copyAction->isEnabled();
                         },
                         15000),
                 "The copy never completed");

        QVERIFY2(QDir(mudlet::getMudletPath(enums::profileHomePath, mQuietCopyName)).exists(), "The copy has no folder on disk");
        QCOMPARE(dialog->readProfileData(mQuietCopyName, qsl("url")), mProfileUrl);
        QCOMPARE(dialog->readProfileData(mQuietCopyName, qsl("port")), mProfilePort);
        auto* pCurrentItem = dialog->listWidget_profiles->currentItem();
        QVERIFY2(pCurrentItem, "Nothing is selected after the copy finished");
        QCOMPARE(pCurrentItem->data(dlgConnectionProfiles::csmNameRole).toString(), mQuietCopyName);
        // still the marked item, so nothing rebuilt the list and this really is
        // the branch the other test cannot reach
        QVERIFY2(pCurrentItem->data(scmTestMarkerRole).toBool(), "The list was rebuilt after all - this test no longer covers the undisturbed branch");

        QDir(mudlet::getMudletPath(enums::profileHomePath, mQuietCopyName)).removeRecursively();
        QDir(mudlet::getMudletPath(enums::profileHomePath, mQuietProfileName)).removeRecursively();
        dialog->fillout_form();
        QTest::qWait(100ms);
    }

    // Has to stay last: it leaves the main window hidden and no connection
    // dialog behind, which every other test here needs.
    // C15: closing the last profile calls slot_showConnectionDialog(), which
    // queues a lambda to show the dialog on the next pass of the event loop.
    // Quitting first takes closeEvent() through closing the dialog (it is
    // WA_DeleteOnClose) and clearing the QPointer to it, leaving the lambda
    // with nothing to show.
    void test_connectionDialogClosedBeforeItIsShown()
    {
        auto* mudletApp = mudlet::self();
        if (mudletApp->mpConnectionDialog) {
            mudletApp->mpConnectionDialog->close();
            mudletApp->mpConnectionDialog = nullptr;
            QTest::qWait(200ms);
        }
        QVERIFY2(!mudletApp->mpConnectionDialog, "Could not get rid of the connection dialog this test starts from");

        mudletApp->slot_showConnectionDialog();
        QVERIFY2(mudletApp->mpConnectionDialog, "No connection dialog was created");

        // exactly what mudlet::closeEvent() does - close the dialog, clear the
        // pointer, hide the main window - and the event loop has not run since
        // the lambda was queued
        QVERIFY2(mudletApp->isVisible(), "The main window has to start out visible for the hide() below to mean anything");
        mudletApp->mpConnectionDialog->close();
        mudletApp->mpConnectionDialog = nullptr;
        mudletApp->hide();
        QVERIFY2(!mudletApp->isVisible(), "The main window did not hide");

        QTest::qWait(300ms); // the queued lambda gets its turn in here

        QVERIFY2(!mudletApp->mpConnectionDialog, "The queued lambda brought the connection dialog back");
        // without the guard the lambda also re-showed the main window, undoing
        // the hide() that closeEvent() had just done
        QVERIFY2(!mudletApp->isVisible(), "The queued lambda re-showed the main window Mudlet was shutting down");
    }
};

QTEST_MAIN(ConnectionDialogCrashTest)
#include "ConnectionDialogCrashTest.moc"
