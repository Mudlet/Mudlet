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
    QString mPort;
    const QString mLocalhost = qsl("localhost");

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
            delete mudlet::self();
            QDir(path).removeRecursively();
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
};

#include "EditorReshowTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorReshowTest)
