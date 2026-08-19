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
 * Crashes of the connection dialog, driven through the real dialog against an
 * isolated config directory. Reaching the end of a test is most of what it
 * asserts; the rest pins the behaviour that replaced the crash.
 *
 * Run with: ctest -R ConnectionDialogCrashTest -V
 */

#include "PortableModeTestHelper.h"
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

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogCrashTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // not created in initTestCase(): the first test needs a profiles/ with
    // nothing in it at all
    const QString mProfileName = qsl("ConnDialogCrash-Test");
    // what copyProfileWidget() names the copy of a name not ending in a digit
    const QString mCopyName = qsl("ConnDialogCrash-Test1");
    const QString mQuietProfileName = qsl("ConnDialogCrash-Quiet");
    const QString mQuietCopyName = qsl("ConnDialogCrash-Quiet1");

    static constexpr int scmMyGamesTab = 0;
    static constexpr int scmAllGamesTab = 1;
    static constexpr int scmTestMarkerRole = Qt::UserRole + 99;
    const QString mProfileUrl = qsl("mudlet.org");
    const QString mProfilePort = qsl("23");

    // by name: QTabWidget gives the dialog a second QTabBar
    QTabBar* gamesTabBar(dlgConnectionProfiles* dialog) const { return dialog->findChild<QTabBar*>(qsl("gamesTabBar")); }

    // members, not locals of the calling test: with no menu the timer is still
    // armed when that method returns
    QStringList mMenuActionTexts;
    bool mSawMenu = false;
    QString mUnexpectedPopup;

    // menu.exec() runs its own event loop, so the menu can only be inspected and
    // dismissed from inside it; callers stop the timer when no menu appears
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
                // menu.exec() waits on whatever holds the popup, so close it
                // rather than time the run out
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

    void disarmMenuCloser(QTimer* closer)
    {
        closer->stop();
        closer->deleteLater();
    }

    QString menuOutcome() const { return mUnexpectedPopup.isEmpty() ? QString() : qsl(" (a %1 took the popup instead)").arg(mUnexpectedPopup); }

    // Must go to the viewport: QAbstractScrollArea ignores a mouse-reason
    // context menu sent to itself, and its viewportEvent() is what raises
    // customContextMenuRequested.
    void rightClickBelowTheLastItem(QAbstractScrollArea* view) const
    {
        auto* viewport = view->viewport();
        const QPoint pos(viewport->width() / 2, viewport->height() - 4);
        QContextMenuEvent event(QContextMenuEvent::Mouse, pos, viewport->mapToGlobal(pos));
        QApplication::sendEvent(viewport, &event);
    }

    // reports instead of QVERIFYing: a QVERIFY here would only leave the helper
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

        // not an empty list: a debug build still lists the self-test entry, but
        // none of it is on disk so fillout_form() makes nothing current
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

        dialog->fillout_form();
        QTest::qWait(100ms);
    }

    // so that "return early when nothing is current" cannot become "return early"
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
        // clear slot_itemClicked()'s 100ms same-profile debounce, which would
        // otherwise leave the form blank after fillout_form() cleared it
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
        // the copy reports back through the event loop, which has not run since,
        // so this destroys the copy's item before the handler sees it
        tabBar->setCurrentIndex(scmAllGamesTab);

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
        // No assertions on the form fields: a copy completes inside
        // slot_itemClicked()'s 100ms debounce, which swallows the fill and
        // leaves Server address and Port blank - a separate bug.
        auto* pCurrentItem = dialog->listWidget_profiles->currentItem();
        QVERIFY2(pCurrentItem, "Nothing is selected after the copy finished");
        QCOMPARE(pCurrentItem->data(dlgConnectionProfiles::csmNameRole).toString(), mCopyName);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mCopyName)).removeRecursively();
        QDir(mudlet::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        dialog->fillout_form();
        QTest::qWait(100ms);
    }

    // the branch where the copy's item is still there and still current
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
        // a mark outlives a rebuild check; a pointer would have been freed by one
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
        QVERIFY2(pCurrentItem->data(scmTestMarkerRole).toBool(), "The list was rebuilt after all - this test no longer covers the undisturbed branch");

        QDir(mudlet::getMudletPath(enums::profileHomePath, mQuietCopyName)).removeRecursively();
        QDir(mudlet::getMudletPath(enums::profileHomePath, mQuietProfileName)).removeRecursively();
        dialog->fillout_form();
        QTest::qWait(100ms);
    }

    // Must stay last: leaves the main window hidden and no connection dialog,
    // both of which the other tests need.
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

        // what closeEvent() does, with the event loop not having run since
        // slot_showConnectionDialog() queued its lambda
        QVERIFY2(mudletApp->isVisible(), "The main window has to start out visible for the hide() below to mean anything");
        mudletApp->mpConnectionDialog->close();
        mudletApp->mpConnectionDialog = nullptr;
        mudletApp->hide();
        QVERIFY2(!mudletApp->isVisible(), "The main window did not hide");

        QTest::qWait(300ms); // the queued lambda gets its turn in here

        QVERIFY2(!mudletApp->mpConnectionDialog, "The queued lambda brought the connection dialog back");
        QVERIFY2(!mudletApp->isVisible(), "The queued lambda re-showed the main window Mudlet was shutting down");
    }
};

#include "ConnectionDialogCrashTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogCrashTest)
