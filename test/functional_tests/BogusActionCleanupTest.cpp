/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include <QtTest/QtTest>

#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include "ActionUnit.h"
#include "Host.h"
#include "TAction.h"
#include "TTreeWidget.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

static void initializeQRCResources() {
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

class BogusActionCleanupTest : public QObject {
  Q_OBJECT

private:
  TelnetServerStub *mpServer = nullptr;
  dlgTriggerEditor *mpEditor = nullptr;
  Host *mpHost = nullptr;
  const QString mProfileName = qsl("BogusActionCleanup-Test-Profile");
  const QString mPort = qsl("23457");
  const QString mLocalhost = qsl("localhost");

  // Qt tests run in the C locale, so these are the literal strings the
  // editor's tr() calls produce when we reproduce the bug below.
  const QString mToolbarName = qsl("New toolbar");
  const QString mMenuName = qsl("New menu");

  void deleteProfileDirectory(const QString &profileName) {
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, profileName);
    QDir dir(path);
    if (dir.exists()) {
      dir.removeRecursively();
    }
  }

  void startProfile(const QString &profileName, const QString &address,
                    const QString &port) {
    QTimer::singleShot(0, qApp, [profileName, address, port]() {
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);

      Q_ASSERT_X(mudlet::self()->mpConnectionDialog, "startProfile",
                 "Connection dialog not initialized");
      Q_ASSERT_X(mudlet::self()->mpConnectionDialog->new_profile_button,
                 "startProfile", "New profile button not found");

      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
      QTest::qWait(100);

      Q_ASSERT_X(QApplication::focusWidget(), "startProfile",
                 "No widget has focus after clicking new profile button");

      QTest::keyClicks(QApplication::focusWidget(), profileName);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), address);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), port);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
    });

    QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
    if (!spy.wait(2000)) {
      QFAIL("Profile took too long to load.");
    }

    mpHost = mudlet::self()->getActiveHost();
    if (!mpHost) {
      QFAIL("No active host available for the test.");
    }

    QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(1000)) {
      QFAIL("Could not connect with the host.");
    }
  }

  // The cleanup slot shows a modal QMessageBox and blocks on exec(); poll
  // briefly for it and click the requested button so the test can progress.
  void clickPendingModal(QMessageBox::StandardButton button) {
    auto *timer = new QTimer(this);
    timer->setInterval(20);
    connect(timer, &QTimer::timeout, this, [timer, button]() {
      QWidget *modal = QApplication::activeModalWidget();
      auto *mbox = qobject_cast<QMessageBox *>(modal);
      if (!mbox) {
        return;
      }
      QAbstractButton *target = mbox->button(button);
      if (!target) {
        target = mbox->defaultButton();
      }
      if (target) {
        target->click();
      } else {
        mbox->reject();
      }
      timer->stop();
      timer->deleteLater();
    });
    timer->start();
  }

  void clearAllActions() {
    auto *pUnit = mpHost->getActionUnit();
    const auto roots = pUnit->getActionRootNodeList();
    for (TAction *pRoot : roots) {
      delete pRoot;
    }
    while (mpEditor->mpActionBaseItem->childCount() > 0) {
      delete mpEditor->mpActionBaseItem->takeChild(0);
    }
    mpEditor->mpCurrentActionItem = nullptr;
    mpEditor->treeWidget_actions->clearSelection();
    mpEditor->treeWidget_actions->setCurrentItem(nullptr);
    mpEditor->mBogusActionsNotified = false;
    mpEditor->hideSystemMessageArea();
  }

  // Reproduce the bug's structure: addNewAction(true) twice, with the first
  // call leaving the new item current so the second nests inside it.
  void createBogusPair() {
    mpEditor->slot_showActions();
    mpEditor->treeWidget_actions->clearSelection();
    mpEditor->treeWidget_actions->setCurrentItem(nullptr);
    mpEditor->addNewAction(true);
    mpEditor->addNewAction(true);
  }

  TAction *firstRootAction() const {
    const auto &roots = mpHost->getActionUnit()->getActionRootNodeList();
    return roots.empty() ? nullptr : roots.front();
  }

  QList<TAction *> scanRoots() const {
    return dlgTriggerEditor::collectBogusActionEntries(
        mpHost->getActionUnit()->getActionRootNodeList(), mToolbarName,
        mMenuName);
  }

private slots:
  void initTestCase() {
    initializeQRCResources();

    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    QVERIFY2(mpServer->isListening(),
             qPrintable(qsl("TelnetServerStub failed to start: %1")
                            .arg(mpServer->errorString())));

    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mProfileName);
    startProfile(mProfileName, mLocalhost, mPort);

    mudlet::self()->slot_showScriptDialog();
    QTest::qWait(100);

    mpEditor = mpHost->mpEditorDialog;
    QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
  }

  void cleanupTestCase() {
    mpEditor = nullptr;
    mpHost = nullptr;
    delete mpServer;
    mpServer = nullptr;
    deleteProfileDirectory(mProfileName);
    delete mudlet::self();
  }

  void init() {
    QVERIFY(mpEditor);
    clearAllActions();
  }

  void cleanup() { clearAllActions(); }

  // ------------------------------------------------------------------
  // Scanner - positive cases
  // ------------------------------------------------------------------
  void testScannerDetectsBogusPair() {
    createBogusPair();

    const auto &roots = mpHost->getActionUnit()->getActionRootNodeList();
    QCOMPARE(roots.size(), size_t{1});
    TAction *pRoot = roots.front();
    QCOMPARE(pRoot->getName(), mToolbarName);
    QVERIFY(pRoot->getChildrenList());
    QCOMPARE(pRoot->getChildrenList()->size(), size_t{1});
    QCOMPARE(pRoot->getChildrenList()->front()->getName(), mMenuName);

    QVERIFY(dlgTriggerEditor::matchesBogusActionSignature(pRoot, mToolbarName,
                                                          mMenuName));

    const auto matches = scanRoots();
    QCOMPARE(matches.size(), qsizetype{1});
    QCOMPARE(matches.front(), pRoot);
  }

  // Empty name args disable the name check - useful if we ever need to
  // clean up profiles saved in a non-English locale.
  void testScannerMatchesWithoutNameFilter() {
    createBogusPair();

    const auto matches = dlgTriggerEditor::collectBogusActionEntries(
        mpHost->getActionUnit()->getActionRootNodeList(), QString(), QString());
    QCOMPARE(matches.size(), qsizetype{1});
  }

  // ------------------------------------------------------------------
  // Scanner - negative cases (each mutates one discriminator)
  // ------------------------------------------------------------------
  void testScannerIgnoresSingleToolbarWithoutChild() {
    mpEditor->slot_showActions();
    mpEditor->treeWidget_actions->clearSelection();
    mpEditor->treeWidget_actions->setCurrentItem(nullptr);
    mpEditor->addNewAction(true);

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresToolbarWithScript() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->setScript(qsl("echo('hi')"));

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresToolbarWithCommand() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->setCommandButtonUp(qsl("look"));

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresActiveToolbar() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->setIsActive(true);

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresPushDownToolbar() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->setIsPushDownButton(true);

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresPackagedToolbar() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->mPackageName = qsl("some-package");

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresToolbarWithExtraChild() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    auto *pExtra = new TAction(pRoot, mpHost);
    pExtra->setName(qsl("Real menu"));
    pExtra->setIsFolder(true);
    pExtra->registerAction();

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresMenuWithGrandchild() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    TAction *pMenu = pRoot->getChildrenList()->front();
    auto *pGrandchild = new TAction(pMenu, mpHost);
    pGrandchild->setName(qsl("Real button"));
    pGrandchild->registerAction();

    QVERIFY(scanRoots().isEmpty());
  }

  void testScannerIgnoresRenamedToolbar() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->setName(qsl("My real toolbar"));

    QVERIFY(scanRoots().isEmpty());
  }

  // Temporary actions (from Lua, not persisted) must never be flagged.
  void testScannerIgnoresTemporaryToolbar() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    pRoot->setTemporary(true);

    QVERIFY(scanRoots().isEmpty());
  }

  // Guard against a refactor that drops the child-side imprint check.
  void testScannerIgnoresChildWithScript() {
    createBogusPair();
    TAction *pRoot = firstRootAction();
    QVERIFY(pRoot);
    QVERIFY(pRoot->getChildrenList() && !pRoot->getChildrenList()->empty());
    TAction *pChild = pRoot->getChildrenList()->front();
    pChild->setScript(qsl("echo('not bogus')"));

    QVERIFY(scanRoots().isEmpty());
  }

  // ------------------------------------------------------------------
  // Banner - detection + user notification
  // ------------------------------------------------------------------
  void testBannerShownWhenBogusPresent() {
    createBogusPair();

    mpEditor->checkForBogusActionsAndNotify();

    QVERIFY(mpEditor->mpSystemMessageArea->isVisible());
    const QString bannerText =
        mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text();
    QVERIFY2(bannerText.contains(qsl("mudlet:cleanupBogusActions")),
             qPrintable(
                 qsl("Banner text missing cleanup link: %1").arg(bannerText)));
    QVERIFY(mpEditor->mBogusActionsNotified);
  }

  void testBannerNotShownWhenClean() {
    mpEditor->checkForBogusActionsAndNotify();
    QVERIFY(!mpEditor->mBogusActionsNotified);
  }

  void testBannerOnlyShownOnce() {
    createBogusPair();
    mpEditor->checkForBogusActionsAndNotify();
    QVERIFY(mpEditor->mBogusActionsNotified);
    mpEditor->hideSystemMessageArea();

    mpEditor->checkForBogusActionsAndNotify();
    QVERIFY(!mpEditor->mpSystemMessageArea->isVisible());
  }

  // ------------------------------------------------------------------
  // Cleanup - full end-to-end, including modal confirmation
  // ------------------------------------------------------------------
  void testCleanupRemovesEntries() {
    createBogusPair();
    QCOMPARE(mpHost->getActionUnit()->getActionRootNodeList().size(),
             size_t{1});
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 1);

    clickPendingModal(QMessageBox::Yes);
    mpEditor->slot_cleanupBogusActions();

    QVERIFY2(mpHost->getActionUnit()->getActionRootNodeList().empty(),
             "Bogus TAction pair should be removed from the action unit");
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 0);
    QVERIFY(!mpEditor->mpSystemMessageArea->isVisible());
  }

  void testCleanupCancelledKeepsEntries() {
    createBogusPair();

    clickPendingModal(QMessageBox::No);
    mpEditor->slot_cleanupBogusActions();

    QCOMPARE(mpHost->getActionUnit()->getActionRootNodeList().size(),
             size_t{1});
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 1);
  }

  // User must get feedback that their click was received, even if the
  // stray entries are already gone by then.
  void testCleanupWithNothingToDoTellsUser() {
    mpEditor->showInfo(qsl("stale"));
    QVERIFY(mpEditor->mpSystemMessageArea->isVisible());

    mpEditor->slot_cleanupBogusActions();

    QVERIFY(mpEditor->mpSystemMessageArea->isVisible());
    const QString bannerText =
        mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text();
    QVERIFY2(
        bannerText.contains(qsl("no longer present"), Qt::CaseInsensitive) ||
            bannerText.contains(qsl("nothing to clean up"),
                                Qt::CaseInsensitive),
        qPrintable(
            qsl("Banner should explain that there was nothing to do, got: %1")
                .arg(bannerText)));
  }

  // Error/warning banners must not be clobbered; the notified flag must
  // stay false so the next showEvent retries once they're dismissed.
  void testBannerDoesNotClobberErrorBanner() {
    createBogusPair();
    mpEditor->showError(qsl("pretend profile-load compile error"));
    QVERIFY(mpEditor->mpSystemMessageArea->isVisible());
    QVERIFY(mpEditor->mpSystemMessageArea->notificationAreaIconLabelError
                ->isVisible());

    mpEditor->checkForBogusActionsAndNotify();

    QCOMPARE(mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text(),
             qsl("pretend profile-load compile error"));
    QVERIFY(mpEditor->mpSystemMessageArea->notificationAreaIconLabelError
                ->isVisible());
    QVERIFY(!mpEditor->mBogusActionsNotified);
  }

  void testBannerDoesNotClobberWarningBanner() {
    createBogusPair();
    mpEditor->showWarning(qsl("pretend warning"), false);
    QVERIFY(mpEditor->mpSystemMessageArea->notificationAreaIconLabelWarning
                ->isVisible());

    mpEditor->checkForBogusActionsAndNotify();

    QCOMPARE(mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text(),
             qsl("pretend warning"));
    QVERIFY(!mpEditor->mBogusActionsNotified);
  }

  // Tree row gone but TAction still on the unit: cleanup must still remove
  // the TAction and surface the drift to the user.
  void testCleanupSurvivesMissingTreeItem() {
    createBogusPair();
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 1);
    delete mpEditor->mpActionBaseItem->takeChild(0);
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 0);
    QCOMPARE(mpHost->getActionUnit()->getActionRootNodeList().size(),
             size_t{1});

    clickPendingModal(QMessageBox::Yes);
    mpEditor->slot_cleanupBogusActions();

    QVERIFY2(mpHost->getActionUnit()->getActionRootNodeList().empty(),
             "Bogus TAction should still be removed from the action unit");
    QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(),
             "User should see a warning rather than a silent success");
    QVERIFY(mpEditor->mpSystemMessageArea->notificationAreaIconLabelWarning
                ->isVisible());
  }

  // The banner link dispatches through slot_clickedMessageBox; make sure
  // that entry point triggers the same flow.
  void testLinkDispatchTriggersCleanup() {
    createBogusPair();

    clickPendingModal(QMessageBox::Yes);
    mpEditor->slot_clickedMessageBox(qsl("mudlet:cleanupBogusActions"));

    QVERIFY(mpHost->getActionUnit()->getActionRootNodeList().empty());
  }

  // Multiple pairs accumulate after several loads of a broken profile;
  // also exercises the %n > 1 plural strings.
  void testScannerAndCleanupHandleMultiplePairs() {
    createBogusPair();
    createBogusPair();
    QCOMPARE(mpHost->getActionUnit()->getActionRootNodeList().size(),
             size_t{2});
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 2);

    const auto matches = mpEditor->findBogusActionEntries();
    QCOMPARE(matches.size(), qsizetype{2});

    clickPendingModal(QMessageBox::Yes);
    mpEditor->slot_cleanupBogusActions();

    QVERIFY2(mpHost->getActionUnit()->getActionRootNodeList().empty(),
             "Both bogus pairs should be removed from the action unit");
    QCOMPARE(mpEditor->mpActionBaseItem->childCount(), 0);
  }

  // showEvent -> QTimer::singleShot is what actually surfaces the banner;
  // direct calls in other tests don't exercise that wiring.
  void testShowEventTriggersBannerCheck() {
    createBogusPair();
    QVERIFY(!mpEditor->mBogusActionsNotified);
    mpEditor->hideSystemMessageArea();

    mpEditor->hide();
    mpEditor->show();
    QTest::qWait(50);

    QVERIFY2(mpEditor->mBogusActionsNotified,
             "showEvent-driven scan must set the notified flag when bogus "
             "entries exist");
    QVERIFY(mpEditor->mpSystemMessageArea->isVisible());
  }
};

#include "BogusActionCleanupTest.moc"
QTEST_MAIN(BogusActionCleanupTest)
