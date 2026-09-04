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
 * Dragging an item onto a folder in the editor's tree is what reparents the
 * underlying trigger, alias or timer - the tree widget watches its own model
 * for the rows moving and pushes the change into the unit. Lua has no way to
 * reparent an item at all, so none of this is reachable from a spec.
 */

#include <QDragEnterEvent>
#include <QMimeData>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "AliasUnit.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAlias.h"
#include "TTimer.h"
#include "TTreeWidget.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TreeWidgetItemMoveTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("TreeWidgetMove-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    QTreeWidgetItem* itemNamed(QTreeWidget* tree, const QString& name) const
    {
        const QList<QTreeWidgetItem*> found = tree->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return found.isEmpty() ? nullptr : found.first();
    }

    // The reparenting only runs while the tree believes a drag is in flight,
    // and a synthetic QDropEvent cannot stand in for one: QAbstractItemView's
    // internal-move handling needs event->source() to be the view itself, which
    // no hand-made event can claim. Announcing the drag and then moving the row
    // through the item API reaches the same two model hooks that a real drop does.
    void moveOntoFolder(TTreeWidget* tree, QTreeWidgetItem* item, QTreeWidgetItem* folder)
    {
        QMimeData mimeData;
        QDragEnterEvent enterEvent(QPoint(1, 1), Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        tree->dragEnterEvent(&enterEvent);

        QTreeWidgetItem* previousParent = item->parent();
        previousParent->takeChild(previousParent->indexOfChild(item));
        folder->insertChild(0, item);

        QDragLeaveEvent leaveEvent;
        tree->dragLeaveEvent(&leaveEvent);
    }

    void populateProfile()
    {
        auto* pTriggerFolder = new TTrigger(nullptr, mpHost);
        pTriggerFolder->setName(qsl("qaMoveTriggerFolder"));
        pTriggerFolder->setIsFolder(true);
        QVERIFY(pTriggerFolder->registerTrigger());

        auto* pTrigger = new TTrigger(nullptr, mpHost);
        pTrigger->setName(qsl("qaMoveTrigger"));
        pTrigger->setRegexCodeList({qsl("^qaMoveTrigger$")}, {REGEX_PERL});
        QVERIFY(pTrigger->registerTrigger());

        auto* pAliasFolder = new TAlias(qsl("qaMoveAliasFolder"), mpHost);
        pAliasFolder->setIsFolder(true);
        QVERIFY(mpHost->getAliasUnit()->registerAlias(pAliasFolder));

        auto* pAlias = new TAlias(qsl("qaMoveAlias"), mpHost);
        pAlias->setRegexCode(qsl("^qaMoveAlias$"));
        QVERIFY(mpHost->getAliasUnit()->registerAlias(pAlias));

        auto* pTimerFolder = new TTimer(qsl("qaMoveTimerFolder"), QTime(0, 0, 1), mpHost);
        pTimerFolder->setIsFolder(true);
        QVERIFY(mpHost->getTimerUnit()->registerTimer(pTimerFolder));

        auto* pTimer = new TTimer(qsl("qaMoveTimer"), QTime(0, 0, 30), mpHost);
        QVERIFY(mpHost->getTimerUnit()->registerTimer(pTimer));
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
        QVERIFY2(connectedSpy.wait(1000), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        // The editor built its trees while the profile loaded, so items added
        // behind its back only appear after the rebuild Host asks for when a
        // package is installed - and that rebuild is only queued
        mpEditor->doCleanReset();
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        if (mpHost) {
            if (auto* pEditor = mpHost->mpEditorDialog.data()) {
                mpHost->mpEditorDialog = nullptr;
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

    void test_droppingATriggerOnAFolderReparentsItInTheUnit()
    {
        mpEditor->slot_showTriggers();
        auto* tree = mpEditor->treeWidget_triggers;
        QTreeWidgetItem* folderItem = itemNamed(tree, qsl("qaMoveTriggerFolder"));
        QTreeWidgetItem* triggerItem = itemNamed(tree, qsl("qaMoveTrigger"));
        QVERIFY(folderItem && triggerItem);

        TTrigger* pFolder = mpHost->getTriggerUnit()->getTrigger(folderItem->data(0, Qt::UserRole).toInt());
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(triggerItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pFolder && pTrigger);
        QVERIFY2(!pTrigger->getParent(), "the trigger was already inside something before the move");

        QSignalSpy movedSpy(tree, &TTreeWidget::itemMoved);
        moveOntoFolder(tree, triggerItem, folderItem);

        QCOMPARE(pTrigger->getParent(), pFolder);
        QCOMPARE(movedSpy.count(), 1);
        const QList<QVariant> arguments = movedSpy.first();
        QCOMPARE(arguments.at(0).toInt(), pTrigger->getID());
        QCOMPARE(arguments.at(1).toInt(), 0); // it had no parent
        QCOMPARE(arguments.at(2).toInt(), pFolder->getID());

        QCOMPARE(folderItem->childCount(), 1);
        QCOMPARE(folderItem->child(0), triggerItem);
        QVERIFY2(!triggerItem->icon(0).isNull(), "the moved trigger was left with no icon");
    }

    void test_droppingAnAliasOnAFolderReparentsItInTheUnit()
    {
        mpEditor->slot_showAliases();
        auto* tree = mpEditor->treeWidget_aliases;
        QTreeWidgetItem* folderItem = itemNamed(tree, qsl("qaMoveAliasFolder"));
        QTreeWidgetItem* aliasItem = itemNamed(tree, qsl("qaMoveAlias"));
        QVERIFY(folderItem && aliasItem);

        TAlias* pFolder = mpHost->getAliasUnit()->getAlias(folderItem->data(0, Qt::UserRole).toInt());
        TAlias* pAlias = mpHost->getAliasUnit()->getAlias(aliasItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pFolder && pAlias);
        QVERIFY2(!pAlias->getParent(), "the alias was already inside something before the move");

        QSignalSpy movedSpy(tree, &TTreeWidget::itemMoved);
        moveOntoFolder(tree, aliasItem, folderItem);

        QCOMPARE(pAlias->getParent(), pFolder);
        QCOMPARE(movedSpy.count(), 1);
    }

    void test_droppingATimerOnAFolderReparentsItAndRepaintsIt()
    {
        mpEditor->slot_showTimers();
        auto* tree = mpEditor->treeWidget_timers;
        QTreeWidgetItem* folderItem = itemNamed(tree, qsl("qaMoveTimerFolder"));
        QTreeWidgetItem* timerItem = itemNamed(tree, qsl("qaMoveTimer"));
        QVERIFY(folderItem && timerItem);

        TTimer* pFolder = mpHost->getTimerUnit()->getTimer(folderItem->data(0, Qt::UserRole).toInt());
        TTimer* pTimer = mpHost->getTimerUnit()->getTimer(timerItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pFolder && pTimer);
        QVERIFY2(!pTimer->getParent(), "the timer was already inside something before the move");
        // Only the active-timer icons are exercised here: the inactive one is
        // painted from an icon path that does not exist (#10401)
        pFolder->setShouldBeActive(true);
        pTimer->setShouldBeActive(true);

        QSignalSpy movedSpy(tree, &TTreeWidget::itemMoved);
        moveOntoFolder(tree, timerItem, folderItem);

        QCOMPARE(pTimer->getParent(), pFolder);
        QCOMPARE(movedSpy.count(), 1);
        QVERIFY2(!folderItem->child(0)->icon(0).isNull(), "the moved timer was left with no icon");
    }

    // Outside a drag the tree is a plain view, so rearranging it must not write
    // anything back into the units
    void test_movingARowWithNoDragInFlightChangesNothing()
    {
        mpEditor->slot_showTriggers();
        auto* tree = mpEditor->treeWidget_triggers;
        QTreeWidgetItem* folderItem = itemNamed(tree, qsl("qaMoveTriggerFolder"));
        QTreeWidgetItem* triggerItem = folderItem->child(0);
        QVERIFY(triggerItem);
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(triggerItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pTrigger);
        TTrigger* parentBefore = pTrigger->getParent();

        QSignalSpy movedSpy(tree, &TTreeWidget::itemMoved);
        folderItem->takeChild(0);
        mpEditor->mpTriggerBaseItem->insertChild(0, triggerItem);

        QCOMPARE(movedSpy.count(), 0);
        QCOMPARE(pTrigger->getParent(), parentBefore);
    }

    void test_getAllChildrenWalksTheWholeSubtree()
    {
        auto* tree = mpEditor->treeWidget_triggers;
        QTreeWidgetItem* folderItem = itemNamed(tree, qsl("qaMoveTriggerFolder"));
        QVERIFY(folderItem);

        // The row is nested two levels down so the walk has to recurse to reach
        // it. Outside a drag the tree writes nothing back into the unit, so a
        // bare item is enough and needs no trigger behind it.
        auto* deepItem = new QTreeWidgetItem(folderItem, QStringList{qsl("qaMoveDeepRow")});
        QVERIFY2(deepItem->parent() == folderItem && folderItem->parent() == mpEditor->mpTriggerBaseItem, "the extra row did not end up two levels below the item being walked");

        QList<QTreeWidgetItem*> collected;
        tree->getAllChildren(mpEditor->mpTriggerBaseItem, collected);

        QVERIFY2(collected.first() == mpEditor->mpTriggerBaseItem, "the item asked about was not itself included");
        QVERIFY2(collected.contains(folderItem), "a top-level item was missed");
        QVERIFY2(collected.contains(deepItem), "a nested item was missed");

        delete folderItem->takeChild(folderItem->indexOfChild(deepItem));
    }
};

#include "TreeWidgetItemMoveTest.moc"
MUDLET_GROUPED_TEST_MAIN(TreeWidgetItemMoveTest)
