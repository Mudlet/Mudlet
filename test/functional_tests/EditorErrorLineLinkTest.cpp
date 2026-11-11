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
 * The line number in a Lua error shown on the script editor's banner is a link
 * that puts the caret on that line of the item's script - but only for the item
 * the error came from.
 *
 * Run with: ctest -R EditorErrorLineLinkTest -V
 */

#include <QRegularExpression>
#include <QTextDocumentFragment>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ScriptUnit.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/textrange.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/texteditorwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorErrorLineLinkTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ErrorLineLink-Test-Profile");
    const QString mLocalhost = qsl("localhost");
    // Past the 63 bytes of a chunk name that Lua keeps in a syntax error
    const QString mCyrillicScriptName = QString(qsl("Скрипт")).repeated(7);
    const QString mLongScriptName = qsl("qaBrokenScriptWithAVeryLongNameThatLuaHasToShortenBeforeItPutsItInTheMessage");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    QString bannerText() const { return mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text(); }

    QString lineLink() const
    {
        static const QRegularExpression link(qsl("href='(mudlet-error-line:\\d+)'"));
        return link.match(bannerText()).captured(1);
    }

    size_t caretLine() const
    {
        const size_t caret = mpEditor->mpSourceEditorEdbee->controller()->textSelection()->range(0).caret();
        return mpEditor->mpSourceEditorEdbeeDocument->lineFromOffset(caret);
    }

    QTreeWidgetItem* treeItem(QTreeWidget* tree, const QString& name) const
    {
        const auto found = tree->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return found.isEmpty() ? nullptr : found.constFirst();
    }

    void selectTrigger(const QString& name)
    {
        QTreeWidgetItem* pItem = treeItem(mpEditor->treeWidget_triggers, name);
        QVERIFY2(pItem, qPrintable(qsl("trigger %1 is not in the editor's tree").arg(name)));
        mpEditor->treeWidget_triggers->setCurrentItem(pItem, 0);
        mpEditor->slot_triggerSelected(pItem);
        // restoreEditorState() places the caret from a zero-delay timer
        QTest::qWait(20ms);
    }

    void moveCaretToStart()
    {
        mpEditor->mpSourceEditorEdbee->controller()->moveCaretTo(static_cast<size_t>(0), static_cast<size_t>(0), false);
        QCOMPARE(caretLine(), static_cast<size_t>(0));
    }

    void populateProfile()
    {
        auto* pBroken = new TTrigger(nullptr, mpHost);
        pBroken->setName(qsl("qaBrokenTrigger"));
        pBroken->setRegexCodeList({qsl("qaBroken")}, {REGEX_SUBSTRING});
        pBroken->setScript(qsl("local a = 1\nlocal b = 2\nlocal c = = 3\nlocal d = 4\nlocal e = 5\n"));
        QVERIFY(pBroken->registerTrigger());
        QVERIFY2(!pBroken->getError().isEmpty(), "the broken trigger compiled");

        auto* pUnclosed = new TTrigger(nullptr, mpHost);
        pUnclosed->setName(qsl("qaUnclosedTrigger"));
        pUnclosed->setRegexCodeList({qsl("qaUnclosed")}, {REGEX_SUBSTRING});
        pUnclosed->setScript(qsl("if true then\nlocal a = 1"));
        QVERIFY(pUnclosed->registerTrigger());
        QVERIFY2(!pUnclosed->getError().isEmpty(), "the unclosed trigger compiled");

        auto* pFine = new TTrigger(nullptr, mpHost);
        pFine->setName(qsl("qaFineTrigger"));
        pFine->setRegexCodeList({qsl("qaFine")}, {REGEX_SUBSTRING});
        pFine->setScript(qsl("local a = 1\nlocal b = 2\nlocal c = 3\nlocal d = 4\n"));
        QVERIFY(pFine->registerTrigger());

        auto* pScript = new TScript(mLongScriptName, mpHost);
        mpHost->getScriptUnit()->registerScript(pScript);
        pScript->setScript(qsl("local a = 1\n\nlocal b = = 2\n"));
        QVERIFY2(!pScript->getError().isEmpty(), "the broken script compiled");

        // "Script: " is 8 bytes and each of these letters 2, so Lua's 63 byte cut lands inside one
        auto* pCyrillic = new TScript(mCyrillicScriptName, mpHost);
        mpHost->getScriptUnit()->registerScript(pCyrillic);
        pCyrillic->setScript(qsl("local a = 1\nlocal b = = 2\n"));
        QVERIFY2(!pCyrillic->getError().isEmpty(), "the broken Cyrillic-named script compiled");

        // A folder and its child of the same name, only the child broken
        auto* pTwinFolder = new TTrigger(nullptr, mpHost);
        pTwinFolder->setName(qsl("qaTwin"));
        pTwinFolder->setIsFolder(true);
        pTwinFolder->setScript(qsl("local a = 1\nlocal b = 2\nlocal c = 3\nlocal d = 4\n"));
        QVERIFY(pTwinFolder->registerTrigger());
        auto* pTwinChild = new TTrigger(pTwinFolder, mpHost);
        pTwinChild->setName(qsl("qaTwin"));
        pTwinChild->setRegexCodeList({qsl("qaTwin")}, {REGEX_SUBSTRING});
        pTwinChild->setScript(qsl("local a = 1\nlocal b = = 2\n"));
        QVERIFY(pTwinChild->registerTrigger());
        QVERIFY2(!pTwinChild->getError().isEmpty(), "the broken twin compiled");

        // A folder whose script was broken and is now repaired still holds the old error message,
        // which a same-named child broken the same way repeats word for word
        auto* pRepairedFolder = new TTrigger(nullptr, mpHost);
        pRepairedFolder->setName(qsl("qaRepaired"));
        pRepairedFolder->setIsFolder(true);
        pRepairedFolder->setScript(qsl("local a = 1\nlocal b = = 2\n"));
        QVERIFY(pRepairedFolder->registerTrigger());
        QVERIFY2(!pRepairedFolder->state(), "the folder's broken script compiled");
        QVERIFY(pRepairedFolder->setScript(qsl("local a = 1\nlocal b = 2\n")));
        auto* pRepairedChild = new TTrigger(pRepairedFolder, mpHost);
        pRepairedChild->setName(qsl("qaRepaired"));
        pRepairedChild->setRegexCodeList({qsl("qaRepaired")}, {REGEX_SUBSTRING});
        pRepairedChild->setScript(qsl("local a = 1\nlocal b = = 2\n"));
        QVERIFY(pRepairedChild->registerTrigger());
        QCOMPARE(pRepairedChild->getError(), pRepairedFolder->getError());
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
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(2s), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        mpEditor->doCleanReset();
        QVERIFY2(QTest::qWaitFor([this]() {
                     return treeItem(mpEditor->treeWidget_triggers, qsl("qaBrokenTrigger")) != nullptr;
                 }),
                 "the editor never rebuilt its trees around the items this test planted");
    }

    void cleanupTestCase()
    {
        if (mpHost) {
            if (auto* pEditor = HostDialogs::of(mpHost).mpEditorDialog.data()) {
                HostDialogs::of(mpHost).mpEditorDialog = nullptr;
                delete pEditor;
            }
        }
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void testErrorLineLinkJumpsToTheLine()
    {
        mpEditor->slot_showTriggers();
        selectTrigger(qsl("qaBrokenTrigger"));
        // The open item's own compile error is on the code heading rather than the banner, and the heading jumps
        QCOMPARE(mpEditor->mEditorCompileErrorLine, 3);

        moveCaretToStart();
        mpEditor->jumpToCompileErrorLine();
        QCOMPARE(caretLine(), static_cast<size_t>(2));
    }

    // Toggling a broken item on wraps its own error in an explanation
    void testActivationFailureStillLinks()
    {
        mpEditor->slot_showTriggers();
        selectTrigger(qsl("qaBrokenTrigger"));
        mpEditor->activeToggle_trigger();
        QVERIFY2(bannerText().contains(qsl("qaBrokenTrigger")) && bannerText().contains(qsl("<p>")), qPrintable(bannerText()));
        QCOMPARE(lineLink(), qsl("mudlet-error-line:3"));

        moveCaretToStart();
        emit mpEditor->mpSystemMessageArea->notificationAreaMessageBox->linkActivated(lineLink());
        QCOMPARE(caretLine(), static_cast<size_t>(2));
    }

    // The banner names the line and says what the compiler said there: the heading note can be cut to the room it has
    void testActivationFailureSaysWhatTheCompilerSaid()
    {
        mpEditor->slot_showTriggers();
        selectTrigger(qsl("qaBrokenTrigger"));
        TTrigger* pBroken = mpHost->getTriggerUnit()->findTrigger(qsl("qaBrokenTrigger"));
        QVERIFY2(pBroken != nullptr, "the broken trigger is not in the trigger unit");
        const QString plain = QTextDocumentFragment::fromHtml(pBroken->getError()).toPlainText().simplified();
        const QString compilerSaid = dlgTriggerEditor::csmCompileErrorLineRegex.match(plain).captured(2);
        QVERIFY2(!compilerSaid.isEmpty(), qPrintable(qsl("SETUP: the planted error is not in the form this case reads: %1").arg(plain)));

        mpEditor->activeToggle_trigger();
        const QString shown = QTextDocumentFragment::fromHtml(bannerText()).toPlainText().simplified();
        QVERIFY2(shown.contains(compilerSaid), qPrintable(qsl("the banner reads \"%1\", without what the compiler said: \"%2\"").arg(shown, compilerSaid)));
        QCOMPARE(lineLink(), qsl("mudlet-error-line:3"));
    }

    // Lua reports an unclosed block on the "end" line that the trigger wraps
    // around its script, one past the script's last line
    void testLinePastTheScriptLandsOnItsLastLine()
    {
        mpEditor->slot_showTriggers();
        selectTrigger(qsl("qaUnclosedTrigger"));
        QCOMPARE(mpEditor->mEditorCompileErrorLine, 3);

        moveCaretToStart();
        mpEditor->jumpToCompileErrorLine();
        QCOMPARE(caretLine(), static_cast<size_t>(1));
    }

    // The banner outlives a switch to another item, whose script the line is not in
    void testLinkDoesNothingOnceAnotherItemIsOpen()
    {
        mpEditor->slot_showTriggers();
        selectTrigger(qsl("qaBrokenTrigger"));
        const QString link = lineLink();
        QCOMPARE(link, qsl("mudlet-error-line:3"));

        selectTrigger(qsl("qaFineTrigger"));
        moveCaretToStart();
        emit mpEditor->mpSystemMessageArea->notificationAreaMessageBox->linkActivated(link);
        QCOMPARE(caretLine(), static_cast<size_t>(0));
    }

    // An error from another item, as a folder shows for a broken descendant,
    // names a line of a script that is not open
    void testOtherItemsErrorGetsNoLink()
    {
        mpEditor->slot_showTriggers();
        selectTrigger(qsl("qaFineTrigger"));
        auto* pBroken = mpHost->getTriggerUnit()->findTrigger(qsl("qaBrokenTrigger"));
        QVERIFY(pBroken);
        mpEditor->showError(pBroken->getError());
        QVERIFY2(bannerText().contains(qsl("]:3:")), qPrintable(bannerText()));
        QVERIFY2(lineLink().isEmpty(), qPrintable(bannerText()));
    }

    // The chunk name cannot tell a folder from a child of the same name, so
    // the error has to be the open item's own
    void testSameNamedItemsErrorGetsNoLink()
    {
        mpEditor->slot_showTriggers();
        QTreeWidgetItem* pFolderItem = treeItem(mpEditor->treeWidget_triggers, qsl("qaTwin"));
        QVERIFY(pFolderItem);
        QVERIFY2(pFolderItem->childCount() == 1, "the first qaTwin found is not the folder");
        mpEditor->treeWidget_triggers->setCurrentItem(pFolderItem, 0);
        mpEditor->slot_triggerSelected(pFolderItem);
        QTest::qWait(20ms);
        auto* pChild = mpEditor->getTriggerFromTreeItem(pFolderItem->child(0));
        QVERIFY(pChild);
        mpEditor->showError(pChild->getError());
        QVERIFY2(bannerText().contains(qsl("&quot;Trigger: qaTwin&quot;]:2:")), qPrintable(bannerText()));
        QVERIFY2(lineLink().isEmpty(), qPrintable(bannerText()));
    }

    void testRepairedFolderTakesNoLinkFromItsChild()
    {
        mpEditor->slot_showTriggers();
        QTreeWidgetItem* pFolderItem = treeItem(mpEditor->treeWidget_triggers, qsl("qaRepaired"));
        QVERIFY(pFolderItem);
        QVERIFY2(pFolderItem->childCount() == 1, "the first qaRepaired found is not the folder");
        mpEditor->treeWidget_triggers->setCurrentItem(pFolderItem, 0);
        mpEditor->slot_triggerSelected(pFolderItem);
        QTest::qWait(20ms);
        auto* pChild = mpEditor->getTriggerFromTreeItem(pFolderItem->child(0));
        QVERIFY(pChild);
        mpEditor->showError(pChild->getError());
        QVERIFY2(bannerText().contains(qsl("&quot;Trigger: qaRepaired&quot;]:2:")), qPrintable(bannerText()));
        QVERIFY2(lineLink().isEmpty(), qPrintable(bannerText()));
    }

    void testShortenedNonAsciiScriptNameStillLinks()
    {
        mpEditor->slot_showScripts();
        QTreeWidgetItem* pItem = treeItem(mpEditor->treeWidget_scripts, mCyrillicScriptName);
        QVERIFY(pItem);
        mpEditor->treeWidget_scripts->setCurrentItem(pItem, 0);
        mpEditor->slot_scriptsSelected(pItem);
        QTest::qWait(20ms);
        QCOMPARE(mpEditor->mEditorCompileErrorLine, 2);
    }

    void testShortenedScriptNameStillLinks()
    {
        mpEditor->slot_showScripts();
        QTreeWidgetItem* pItem = treeItem(mpEditor->treeWidget_scripts, mLongScriptName);
        QVERIFY(pItem);
        mpEditor->treeWidget_scripts->setCurrentItem(pItem, 0);
        mpEditor->slot_scriptsSelected(pItem);
        QTest::qWait(20ms);
        QCOMPARE(mpEditor->mEditorCompileErrorLine, 3);

        moveCaretToStart();
        mpEditor->jumpToCompileErrorLine();
        QCOMPARE(caretLine(), static_cast<size_t>(2));
    }
};

#include "EditorErrorLineLinkTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorErrorLineLinkTest)
