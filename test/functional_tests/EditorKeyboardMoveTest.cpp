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
 * Alt+arrow keys in the editor's item tree move the selected item, for anyone
 * who cannot drag it. Lua has no way to press a key in the editor or to
 * reorder items, so none of this is reachable from a spec.
 */

#include <QAccessible>
#include <QFile>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QXmlStreamReader>
#include <QtTest/QtTest>
#include <chrono>

#include "AliasUnit.h"
#include "EditorUndoStack.h"
#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAlias.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "XMLexport.h"
#include "ctelnet.h"
#include "dlgAliasMainArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorKeyboardMoveTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorKeyboardMove-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    // What a screen reader is handed, as Qt calls an installed handler in place of the platform's
    inline static QStringList smHeard;

    static void recordAccessibleEvent(QAccessibleEvent* pEvent)
    {
        if (pEvent->type() == QAccessible::Announcement) {
            smHeard << static_cast<QAccessibleAnnouncementEvent*>(pEvent)->message();
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // The tree is rebuilt after every move, so rows are looked up afresh each time
    QTreeWidgetItem* itemNamed(const QString& name) const
    {
        const QList<QTreeWidgetItem*> found = mpEditor->treeWidget_triggers->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return found.isEmpty() ? nullptr : found.first();
    }

    TTrigger* triggerNamed(const QString& name) const
    {
        QTreeWidgetItem* pItem = itemNamed(name);
        return pItem ? mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt()) : nullptr;
    }

    // Only this test's own, as the profile starts with its default packages' triggers
    QStringList rootTriggerNames() const
    {
        QStringList names;
        for (const TTrigger* pTrigger : mpHost->getTriggerUnit()->getTriggerRootNodeList()) {
            if (pTrigger->getName().startsWith(qsl("kb"))) {
                names << pTrigger->getName();
            }
        }
        return names;
    }

    QStringList childNames(const TTrigger* pParent) const
    {
        QStringList names;
        for (const auto* pChild : *pParent->getChildrenList()) {
            names << static_cast<const TTrigger*>(pChild)->getName();
        }
        return names;
    }

    // Names of the top-level triggers and groups as a profile save would write them
    QStringList exportedRootTriggerNames()
    {
        QTemporaryDir exportDir;
        const QString fileName = exportDir.filePath(qsl("export.xml"));
        auto exporter = std::make_shared<XMLexport>(mpHost);
        if (!exporter->exportPackage(fileName)) {
            return {};
        }
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        QStringList names;
        QXmlStreamReader reader(&file);
        int depthInPackage = -1;
        int depth = 0;
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isStartElement()) {
                ++depth;
                if (reader.name() == qsl("TriggerPackage")) {
                    depthInPackage = depth;
                } else if (depthInPackage > 0 && depth == depthInPackage + 2 && reader.name() == qsl("name")) {
                    names << reader.readElementText();
                    --depth;
                }
            } else if (reader.isEndElement()) {
                if (depth == depthInPackage) {
                    depthInPackage = -1;
                }
                --depth;
            }
        }
        return names;
    }

    void selectInTree(const QString& name)
    {
        QTreeWidgetItem* pItem = itemNamed(name);
        QVERIFY2(pItem, qPrintable(qsl("the trigger tree has no row for %1").arg(name)));
        auto* pTree = mpEditor->treeWidget_triggers;
        pTree->setCurrentItem(pItem);
        mpEditor->slot_triggerSelected(pItem);
        pTree->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY2(pTree->hasFocus(), "the trigger tree never took the keyboard focus");
    }

    void populateProfile()
    {
        auto addTrigger = [this](const QString& name, TTrigger* pParent, const bool isFolder) {
            auto* pTrigger = new TTrigger(pParent, mpHost);
            pTrigger->setName(name);
            pTrigger->setIsFolder(isFolder);
            if (!isFolder) {
                pTrigger->setRegexCodeList({qsl("^%1$").arg(name)}, {REGEX_PERL});
            }
            pTrigger->registerTrigger();
        };
        addTrigger(qsl("kbFirst"), nullptr, false);
        // Listed in the unit but never shown in the tree, so the unit's positions and the tree's differ
        auto* pTemporary = new TTrigger(nullptr, mpHost);
        pTemporary->setName(qsl("kbHiddenTemporary"));
        pTemporary->setRegexCodeList({qsl("^kbHiddenTemporary$")}, {REGEX_PERL});
        pTemporary->setTemporary(true);
        pTemporary->registerTrigger();
        addTrigger(qsl("kbSecond"), nullptr, false);
        auto* pFolder = new TTrigger(nullptr, mpHost);
        pFolder->setName(qsl("kbFolder"));
        pFolder->setIsFolder(true);
        pFolder->registerTrigger();
        addTrigger(qsl("kbInside"), pFolder, false);
        addTrigger(qsl("kbAfterFolder"), nullptr, false);

        for (const QString& name : {qsl("kbAliasOne"), qsl("kbAliasTwo")}) {
            auto* pAlias = new TAlias(name, mpHost);
            pAlias->setRegexCode(qsl("^%1$").arg(name));
            mpHost->getAliasUnit()->registerAlias(pAlias);
        }
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
        QVERIFY2(connectedSpy.wait(1s), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        mpEditor->doCleanReset();
        QVERIFY2(QTest::qWaitFor([this]() {
                     return itemNamed(qsl("kbInside")) != nullptr;
                 }),
                 "the editor never rebuilt its trees around the triggers this test planted");

        mpEditor->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mpEditor));
        mpEditor->slot_showTriggers();
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

    void test_altDownAndUpMoveATriggerPastItsVisibleNeighbour()
    {
        QCOMPARE(rootTriggerNames(), QStringList({qsl("kbFirst"), qsl("kbHiddenTemporary"), qsl("kbSecond"), qsl("kbFolder"), qsl("kbAfterFolder")}));
        QVERIFY2(!itemNamed(qsl("kbHiddenTemporary")), "the temporary trigger is in the tree, so it cannot stand for the items the tree hides");

        selectInTree(qsl("kbFirst"));
        // An edit not yet saved has to survive the form being reloaded after the move
        mpEditor->mpTriggersMainArea->lineEdit_trigger_name->setText(qsl("kbFirstRenamed"));
        QTest::keyClick(mpEditor->treeWidget_triggers, Qt::Key_Down, Qt::AltModifier);

        QCOMPARE(rootTriggerNames(), QStringList({qsl("kbHiddenTemporary"), qsl("kbSecond"), qsl("kbFirstRenamed"), qsl("kbFolder"), qsl("kbAfterFolder")}));
        QTreeWidgetItem* pMoved = itemNamed(qsl("kbFirstRenamed"));
        QTreeWidgetItem* pSibling = itemNamed(qsl("kbSecond"));
        QVERIFY2(pMoved && pSibling, "the tree lost a row in the move");
        QCOMPARE(pMoved->parent(), pSibling->parent());
        QCOMPARE(pMoved->parent()->indexOfChild(pMoved), pSibling->parent()->indexOfChild(pSibling) + 1);
        QVERIFY2(mpEditor->treeWidget_triggers->currentItem() == pMoved, "the moved trigger is no longer the selected one");
        QVERIFY2(mpEditor->treeWidget_triggers->hasFocus(), "the tree lost the focus, so a second Alt+arrow would go elsewhere");

        const QStringList exported = exportedRootTriggerNames();
        QVERIFY2(exported.indexOf(qsl("kbSecond")) >= 0 && exported.indexOf(qsl("kbFirstRenamed")) == exported.indexOf(qsl("kbSecond")) + 1,
                 qPrintable(qsl("the export lists the root triggers as: %1").arg(exported.join(qsl(", ")))));

        QTest::keyClick(mpEditor->treeWidget_triggers, Qt::Key_Up, Qt::AltModifier);
        QCOMPARE(rootTriggerNames(), QStringList({qsl("kbHiddenTemporary"), qsl("kbFirstRenamed"), qsl("kbSecond"), qsl("kbFolder"), qsl("kbAfterFolder")}));

        mpEditor->mpUndoStack->undo();
        mpEditor->mpUndoStack->undo();
        QCOMPARE(rootTriggerNames(), QStringList({qsl("kbFirstRenamed"), qsl("kbHiddenTemporary"), qsl("kbSecond"), qsl("kbFolder"), qsl("kbAfterFolder")}));
    }

    void test_altLeftMovesATriggerOutOfItsFolderAndUndoPutsItBack()
    {
        TTrigger* pFolder = triggerNamed(qsl("kbFolder"));
        TTrigger* pInside = triggerNamed(qsl("kbInside"));
        QVERIFY(pFolder && pInside);
        QCOMPARE(pInside->getParent(), pFolder);

        selectInTree(qsl("kbInside"));
        QTest::keyClick(mpEditor->treeWidget_triggers, Qt::Key_Left, Qt::AltModifier);

        QVERIFY2(!pInside->getParent(), "the trigger is still inside a group");
        const QStringList roots = rootTriggerNames();
        QVERIFY2(roots.indexOf(qsl("kbInside")) == roots.indexOf(qsl("kbFolder")) + 1 && roots.indexOf(qsl("kbAfterFolder")) == roots.indexOf(qsl("kbInside")) + 1,
                 qPrintable(qsl("root triggers are now: %1").arg(roots.join(qsl(", ")))));
        QTreeWidgetItem* pMoved = itemNamed(qsl("kbInside"));
        QVERIFY(pMoved);
        QCOMPARE(pMoved->parent(), mpEditor->mpTriggerBaseItem);

        mpEditor->mpUndoStack->undo();
        QCOMPARE(pInside->getParent(), pFolder);
        QCOMPARE(childNames(pFolder), QStringList{qsl("kbInside")});
    }

    void test_altRightMovesATriggerIntoTheGroupAbove()
    {
        TTrigger* pFolder = triggerNamed(qsl("kbFolder"));
        TTrigger* pAfter = triggerNamed(qsl("kbAfterFolder"));
        QVERIFY(pFolder && pAfter);
        QVERIFY2(!pAfter->getParent(), "the trigger was already inside something before the move");

        selectInTree(qsl("kbAfterFolder"));
        QTest::keyClick(mpEditor->treeWidget_triggers, Qt::Key_Right, Qt::AltModifier);

        QCOMPARE(pAfter->getParent(), pFolder);
        QCOMPARE(childNames(pFolder).last(), qsl("kbAfterFolder"));

        mpEditor->mpUndoStack->undo();
        QVERIFY2(!pAfter->getParent(), "undo left the trigger inside the group");
    }

    // The keys belong to whatever has the focus, so typing in the form must not move anything
    void test_altArrowsOutsideTheTreeMoveNothing()
    {
        selectInTree(qsl("kbSecond"));
        const QStringList before = rootTriggerNames();
        auto* pNameField = mpEditor->mpTriggersMainArea->lineEdit_trigger_name;
        pNameField->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY(pNameField->hasFocus());

        QTest::keyClick(pNameField, Qt::Key_Down, Qt::AltModifier);

        QCOMPARE(rootTriggerNames(), before);
    }

    // Any view other than triggers takes the same path
    void test_altDownMovesAnAliasAndTellsTheScreenReader()
    {
        mpEditor->slot_showAliases();
        auto* pTree = mpEditor->treeWidget_aliases;
        const QList<QTreeWidgetItem*> found = pTree->findItems(qsl("kbAliasOne"), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        QVERIFY2(!found.isEmpty(), "the alias tree has no row for the alias this test planted");
        pTree->setCurrentItem(found.first());
        mpEditor->slot_aliasSelected(found.first());
        pTree->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY2(pTree->hasFocus(), "the alias tree never took the keyboard focus");

        smHeard.clear();
        const bool wasActive = QAccessible::isActive();
        const QAccessible::UpdateHandler previousHandler = QAccessible::installUpdateHandler(recordAccessibleEvent);
        QAccessible::setActive(true);
        QTest::keyClick(pTree, Qt::Key_Down, Qt::AltModifier);
        QAccessible::installUpdateHandler(previousHandler);
        QAccessible::setActive(wasActive);

        QStringList aliasNames;
        for (const TAlias* pAlias : mpHost->getAliasUnit()->getAliasRootNodeList()) {
            if (pAlias->getName().startsWith(qsl("kb"))) {
                aliasNames << pAlias->getName();
            }
        }
        QCOMPARE(aliasNames, QStringList({qsl("kbAliasTwo"), qsl("kbAliasOne")}));
        QVERIFY2(smHeard.contains(qsl("Moved kbAliasOne below kbAliasTwo")), qPrintable(qsl("the screen reader heard: %1").arg(smHeard.join(qsl(" | ")))));
        mpEditor->slot_showTriggers();
    }

    // Rebuilding the tree reloads the form from the unit, so a move must not follow a save the alias refused
    void test_altDownKeepsAnAliasEditThatCannotBeSaved()
    {
        mpEditor->slot_showAliases();
        auto* pTree = mpEditor->treeWidget_aliases;
        const QList<QTreeWidgetItem*> found = pTree->findItems(qsl("kbAliasTwo"), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        QVERIFY2(!found.isEmpty(), "the alias tree has no row for the alias this test planted");
        pTree->setCurrentItem(found.first());
        mpEditor->slot_aliasSelected(found.first());
        pTree->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY2(pTree->hasFocus(), "the alias tree never took the keyboard focus");

        auto aliasNames = [this]() {
            QStringList names;
            for (const TAlias* pAlias : mpHost->getAliasUnit()->getAliasRootNodeList()) {
                if (pAlias->getName().startsWith(qsl("kb"))) {
                    names << pAlias->getName();
                }
            }
            return names;
        };
        const QStringList before = aliasNames();
        QCOMPARE(before.first(), qsl("kbAliasTwo"));
        // Matches the alias's own pattern, so saving refuses it as a loop
        auto* pCommandField = mpEditor->mpAliasMainArea->lineEdit_alias_command;
        pCommandField->setText(qsl("kbAliasTwo"));
        mpEditor->mpAliasMainArea->lineEdit_alias_name->setText(qsl("kbAliasTwoRenamed"));

        QTest::keyClick(pTree, Qt::Key_Down, Qt::AltModifier);

        QCOMPARE(aliasNames(), before);
        QCOMPARE(pCommandField->text(), qsl("kbAliasTwo"));
        QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_name->text(), qsl("kbAliasTwoRenamed"));

        pCommandField->clear();
        mpEditor->mpAliasMainArea->lineEdit_alias_name->setText(qsl("kbAliasTwo"));
        mpEditor->slot_showTriggers();
    }
};

#include "EditorKeyboardMoveTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorKeyboardMoveTest)
