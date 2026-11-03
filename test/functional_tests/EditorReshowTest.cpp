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
 * The script editor is a singleton that every entry point re-shows rather than
 * rebuilds when the profile already has one, so what those paths do to a window
 * the user arranged is what these cover: the state the window was left in
 * (PR #9303), and the case where there is no window left to re-show (PR #7337).
 *
 * Run with: ctest -R EditorReshowTest -V
 */

#include <QAction>
#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include "AliasUnit.h"
#include "Host.h"
#include "HostDialogs.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAlias.h"
#include "TConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorReshowTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorReshow-Test-Profile");
    const QString mSecondProfileName = qsl("EditorReshow-Test-Profile-2");
    const QString mThirdProfileName = qsl("EditorReshow-Test-Profile-3");
    const QString mFourthProfileName = qsl("EditorReshow-Test-Profile-4");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    // The two entry points build the deferred trees by different routes: showEvent() for the Script Editor
    // button, a view switch for the aliases one
    void checkNoticePostedAfterTheDeferredResetSurvivesFirstOpen(const QString& profileName, const bool viaAliases)
    {
        Host* pHost = TestProfile::create(profileName, mLocalhost, mPort);
        QVERIFY2(pHost, "no host for the profile");
        QSignalSpy connected(&(pHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "could not connect the profile to the stub server");
        dlgTriggerEditor* pEditor = HostDialogs::of(pHost).mpEditorDialog;
        QVERIFY2(pEditor && !pEditor->isVisible(), "setup: the profile needs an editor nothing has opened");

        // Hidden, or the view's introduction would replace the notice anyway
        pEditor->setBannerPermanentlyHidden(EditorViewType::cmTriggerView, qsl("intro"), true);
        pEditor->setBannerPermanentlyHidden(EditorViewType::cmAliasView, qsl("intro"), true);
        pEditor->doCleanReset();
        QTest::qWait(50ms);
        QVERIFY2(pEditor->mCleanResetDeferred, "setup: the reset was not put off");
        pEditor->showWarning(qsl("qaNoticeAfterTheDeferredReset"), false);

        QCOMPARE(mudlet::self()->getActiveHost(), pHost);
        if (viaAliases) {
            mudlet::self()->slot_showAliasDialog();
        } else {
            mudlet::self()->slot_showEditorDialog();
        }
        QTest::qWait(50ms);

        QVERIFY2(pEditor->isVisible(), "the entry point did not open the editor");
        QVERIFY2(!pEditor->mCleanResetDeferred, "opening the editor did not build its trees");
        auto* pArea = pEditor->mpSystemMessageArea;
        QVERIFY2(!pArea->isHidden(), "the notice posted after the reset was put off is gone");
        QCOMPARE(pArea->notificationAreaMessageBox->text(), qsl("qaNoticeAfterTheDeferredReset"));
        QVERIFY2(!pArea->notificationAreaIconLabelWarning->isHidden(), "the notice lost its warning icon");
        QVERIFY(pArea->notificationAreaIconLabelInformation->isHidden());
        QVERIFY(pArea->notificationAreaIconLabelError->isHidden());
        QVERIFY(pEditor->mCurrentBannerKey.isEmpty());
        pArea->messageAreaCloseButton->click();
        QVERIFY2(pArea->isHidden(), "the close button no longer dismisses the notice");

        pHost->forceClose();
        QVERIFY2(pHost->requestClose(), "Closing the profile was refused");
        HostManager::self()->deleteHost(profileName);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mSecondProfileName)).removeRecursively();
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mThirdProfileName)).removeRecursively();
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mFourthProfileName)).removeRecursively();
        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "no active host after creating the profile");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "could not connect the profile to the stub server");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mProfileName);
            const QString secondPath = MudletApp::getMudletPath(enums::profileHomePath, mSecondProfileName);
            const QString thirdPath = MudletApp::getMudletPath(enums::profileHomePath, mThirdProfileName);
            const QString fourthPath = MudletApp::getMudletPath(enums::profileHomePath, mFourthProfileName);
            delete mudlet::self();
            QDir(path).removeRecursively();
            QDir(secondPath).removeRecursively();
            QDir(thirdPath).removeRecursively();
            QDir(fourthPath).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Runs first, as it needs an editor nothing has shown: its trees stay empty until an entry point opens it,
    // which then has to find them filled and stay on its own view rather than the triggers one
    void test_anUnopenedEditorFillsItsTreesWhenFirstOpened()
    {
        dlgTriggerEditor* pEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(pEditor, "the profile has no editor to show");
        QVERIFY2(!pEditor->isVisible(), "setup: the editor has to start out never opened");
        auto* pTriggers = pEditor->findChild<QTreeWidget*>(qsl("treeWidget_triggers"));
        auto* pAliases = pEditor->findChild<QTreeWidget*>(qsl("treeWidget_aliases"));
        QVERIFY(pTriggers && pAliases);

        // Made the way a profile load makes it: a script's permAlias() would flag the trees stale itself
        auto* pAlias = new TAlias(nullptr, mpHost);
        mpHost->getAliasUnit()->registerAlias(pAlias);
        pAlias->setName(qsl("qaUnopenedAlias"));
        pEditor->doCleanReset();
        QTest::qWait(50ms);
        QCOMPARE(pTriggers->topLevelItemCount(), 0);
        QCOMPARE(pAliases->topLevelItemCount(), 0);

        mudlet::self()->slot_showAliasDialog();
        QVERIFY2(pEditor->isVisible(), "the alias entry point did not open the editor");
        QTest::qWait(50ms);
        QCOMPARE(pTriggers->topLevelItemCount(), 1);
        const auto found = pAliases->findItems(qsl("qaUnopenedAlias"), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        QVERIFY2(!found.isEmpty(), "the alias made before the editor was opened is missing from its tree");
        QVERIFY2(pAliases->isVisible(), "the editor opened on some other view than the aliases one asked for");
        QVERIFY2(!pTriggers->isVisible(), "the editor opened on the triggers view as well as the aliases one");
        // With no notice waiting, the aliases view's own introduction is what the message area shows
        auto* pMessageArea = pEditor->findChild<QWidget*>(qsl("mpSystemMessageArea"));
        auto* pMessage = pMessageArea ? pMessageArea->findChild<QLabel*>(qsl("notificationAreaMessageBox")) : nullptr;
        QVERIFY(pMessage);
        QVERIFY2(pMessageArea->isVisible(), "the editor opened with its message area hidden");
        QVERIFY2(pMessage->text().contains(dlgTriggerEditor::tr("Alias react on user input.")), qPrintable(pMessage->text()));
    }

    // PR #9303: every editor show path called showNormal(), which forces a
    // window out of a maximized state, so an editor left maximized came back
    // un-maximized the next time the user toggled to it.
    void test_theEditorComesBackMaximized()
    {
        dlgTriggerEditor* pEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(pEditor, "the profile has no editor to show");

        mudlet::self()->slot_showTriggerDialog();
        QVERIFY2(!pEditor->isMaximized(), "the editor was maximized to begin with, so this cannot tell a restored state from the one it started in");

        pEditor->showMaximized();
        QVERIFY2(pEditor->isMaximized(), "the editor would not maximize here, so there is no state for the toggle to lose");

        pEditor->hide();
        mudlet::self()->slot_showTriggerDialog();

        // hiding a window leaves its maximized state alone, so without this the
        // assertion below would hold for a re-show that never happened too
        QVERIFY2(pEditor->isVisible(), "the editor was never re-shown, so its window state proves nothing");
        QVERIFY2(pEditor->isMaximized(), "the editor came back un-maximized");
    }

    // PR #7337: the editor entry points dereferenced the profile's editor
    // without checking it; they now build an editor when the profile has none.
    void test_theErrorsMenuItemRebuildsAMissingEditor()
    {
        QVERIFY(HostDialogs::of(mpHost).mpEditorDialog);
        delete HostDialogs::of(mpHost).mpEditorDialog.data();
        QVERIFY2(HostDialogs::of(mpHost).mpEditorDialog.isNull(), "the profile is still holding an editor, so nothing here has to be rebuilt");
        QVERIFY2(mudlet::self()->dactionShowErrors->isEnabled(), "the Errors menu item is disabled, so triggering it proves nothing");

        mudlet::self()->dactionShowErrors->trigger();

        QVERIFY2(!HostDialogs::of(mpHost).mpEditorDialog.isNull(), "the Errors menu item left the profile with no editor to show the errors in");
        QVERIFY(HostDialogs::of(mpHost).mpEditorDialog->mpErrorConsole);
        QVERIFY2(HostDialogs::of(mpHost).mpEditorDialog->mpErrorConsole->isVisible(), "the rebuilt editor did not open on the error console");
    }

    // The Script Editor button opens an unopened editor on the triggers view its first build sets up, and a
    // notice posted before that build was put off is gone, as when the build ran at load
    void test_theScriptEditorButtonOpensAnUnopenedEditorOnItsTriggers()
    {
        Host* pSecondHost = TestProfile::create(mSecondProfileName, mLocalhost, mPort);
        QVERIFY2(pSecondHost, "no host for the second profile");
        QSignalSpy connected(&(pSecondHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "could not connect the second profile to the stub server");
        dlgTriggerEditor* pEditor = HostDialogs::of(pSecondHost).mpEditorDialog;
        QVERIFY2(pEditor && !pEditor->isVisible(), "setup: the second profile needs an editor nothing has opened");

        // Hidden, or the triggers introduction would replace the notice anyway
        pEditor->setBannerPermanentlyHidden(EditorViewType::cmTriggerView, qsl("intro"), true);
        pEditor->showWarning(qsl("qaNoticeBeforeTheBuild"), false);
        // Filled in by the triggers view's own setup, which a bare switch to the view skips
        pEditor->mTriggerEditorSplitterState.clear();
        pEditor->doCleanReset();
        QTest::qWait(50ms);
        QCOMPARE(mudlet::self()->getActiveHost(), pSecondHost);
        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(50ms);

        QVERIFY2(pEditor->isVisible(), "the Script Editor button did not open the editor");
        QVERIFY2(pEditor->mCurrentView == EditorViewType::cmTriggerView, "the editor did not open on the triggers view");
        QVERIFY2(!pEditor->mTriggerEditorSplitterState.isEmpty(), "the triggers view was switched to without its layout being set up");
        QVERIFY2(pEditor->mpSystemMessageArea->isHidden() || !pEditor->mpSystemMessageArea->notificationAreaMessageBox->text().contains(qsl("qaNoticeBeforeTheBuild")),
                 "a notice the build would have cleared at load is still showing");

        pSecondHost->forceClose();
        QVERIFY2(pSecondHost->requestClose(), "Closing the second profile was refused");
        HostManager::self()->deleteHost(mSecondProfileName);
    }

    void test_aNoticePostedAfterTheDeferredResetSurvivesTheScriptEditorButton() { checkNoticePostedAfterTheDeferredResetSurvivesFirstOpen(mThirdProfileName, false); }

    void test_aNoticePostedAfterTheDeferredResetSurvivesTheAliasesButton() { checkNoticePostedAfterTheDeferredResetSurvivesFirstOpen(mFourthProfileName, true); }
};

#include "EditorReshowTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorReshowTest)
