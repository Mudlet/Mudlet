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
 * The emergency stop switches every trigger, alias, timer and key off at run
 * time but leaves what the player switched on alone, and ending the stop
 * resumes all of that. The editor has to keep showing what the player switched
 * on, and saving an item must not switch it back on before the stop ends.
 */

#include <QDragEnterEvent>
#include <QMimeData>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "AliasUnit.h"
#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "KeyUnit.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAlias.h"
#include "TKey.h"
#include "TTimer.h"
#include "TTreeWidget.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "edbee/models/textdocument.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorEmergencyStopTest : public QObject
{
    Q_OBJECT

private:
    struct Look
    {
        QString description;
        qint64 iconKey = 0;
    };

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorEmergencyStop-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    QTreeWidget* showView(const QString& kind)
    {
        if (kind == qsl("trigger")) {
            mpEditor->slot_showTriggers();
            return mpEditor->treeWidget_triggers;
        }
        if (kind == qsl("alias")) {
            mpEditor->slot_showAliases();
            return mpEditor->treeWidget_aliases;
        }
        if (kind == qsl("timer")) {
            mpEditor->slot_showTimers();
            return mpEditor->treeWidget_timers;
        }
        mpEditor->slot_showKeys();
        return mpEditor->treeWidget_keys;
    }

    void select(const QString& kind, QTreeWidget* pTree, QTreeWidgetItem* pItem)
    {
        pTree->setCurrentItem(pItem);
        if (kind == qsl("trigger")) {
            mpEditor->slot_triggerSelected(pItem);
        } else if (kind == qsl("alias")) {
            mpEditor->slot_aliasSelected(pItem);
        } else if (kind == qsl("timer")) {
            mpEditor->slot_timerSelected(pItem);
        } else {
            mpEditor->slot_keySelected(pItem);
        }
    }

    static QTreeWidgetItem* itemNamed(QTreeWidget* pTree, const QString& name)
    {
        const auto found = pTree->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return found.count() == 1 ? found.first() : nullptr;
    }

    bool running(const QString& kind, QTreeWidgetItem* pItem) const
    {
        const int id = pItem->data(0, Qt::UserRole).toInt();
        if (kind == qsl("trigger")) {
            return mpHost->getTriggerUnit()->getTrigger(id)->isActive();
        }
        if (kind == qsl("alias")) {
            return mpHost->getAliasUnit()->getAlias(id)->isActive();
        }
        if (kind == qsl("timer")) {
            return mpHost->getTimerUnit()->getTimer(id)->isActive();
        }
        return mpHost->getKeyUnit()->getKey(id)->isActive();
    }

    // The reparenting only runs while the tree believes a drag is in flight, and a hand-made
    // QDropEvent cannot claim the view as its source, so the row is moved through the item API
    static void moveOntoFolder(TTreeWidget* pTree, QTreeWidgetItem* pItem, QTreeWidgetItem* pFolder)
    {
        QMimeData mimeData;
        QDragEnterEvent enterEvent(QPoint(1, 1), Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        pTree->dragEnterEvent(&enterEvent);
        QTreeWidgetItem* pOldParent = pItem->parent();
        pOldParent->takeChild(pOldParent->indexOfChild(pItem));
        pFolder->insertChild(0, pItem);
        QDragLeaveEvent leaveEvent;
        pTree->dragLeaveEvent(&leaveEvent);
    }

    static QImage imageOf(QTreeWidgetItem* pItem) { return pItem->icon(0).pixmap(16, 16).toImage(); }

    static Look lookOf(QTreeWidgetItem* pItem) { return {pItem->data(0, Qt::AccessibleDescriptionRole).toString(), pItem->icon(0).cacheKey()}; }

    // Folder, a child left on and a child the player switched off, as the tree shows them
    QList<QTreeWidgetItem*> itemsOf(QTreeWidget* pTree, const QString& prefix)
    {
        return {itemNamed(pTree, prefix + qsl("Folder")), itemNamed(pTree, prefix + qsl("On")), itemNamed(pTree, prefix + qsl("Off"))};
    }

    void addKinds()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("prefix");
        QTest::newRow("trigger") << qsl("trigger") << qsl("qaStopTrigger");
        QTest::newRow("alias") << qsl("alias") << qsl("qaStopAlias");
        QTest::newRow("timer") << qsl("timer") << qsl("qaStopTimer");
        QTest::newRow("key") << qsl("key") << qsl("qaStopKey");
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(1s), "Could not connect with the host.");

        QVERIFY(runLua(qsl(R"(
            permSubstringTrigger("qaStopTriggerFolder", "", {}, "")
            permSubstringTrigger("qaStopTriggerOn", "qaStopTriggerFolder", {"qaStopTriggerOn"}, "")
            permSubstringTrigger("qaStopTriggerOff", "qaStopTriggerFolder", {"qaStopTriggerOff"}, "")
            disableTrigger("qaStopTriggerOff")
            permSubstringTrigger("qaStopTriggerBroken", "qaStopTriggerFolder", {"qaStopTriggerBroken"}, "")
            permSubstringTrigger("qaStopTriggerMoving", "", {"qaStopTriggerMoving"}, "")
            permAlias("qaStopAliasFolder", "", "", "")
            permAlias("qaStopAliasOn", "qaStopAliasFolder", "^qaStopAliasOn$", "")
            permAlias("qaStopAliasOff", "qaStopAliasFolder", "^qaStopAliasOff$", "")
            disableAlias("qaStopAliasOff")
            permAlias("qaStopAliasBroken", "qaStopAliasFolder", "^qaStopAliasBroken$", "")
            permTimer("qaStopTimerFolder", "", 0, "")
            permTimer("qaStopTimerOn", "qaStopTimerFolder", 3600, "")
            permTimer("qaStopTimerOff", "qaStopTimerFolder", 3600, "")
            enableTimer("qaStopTimerFolder")
            enableTimer("qaStopTimerOn")
            disableTimer("qaStopTimerOff")
            permTimer("qaStopTimerBroken", "qaStopTimerFolder", 3600, "")
            enableTimer("qaStopTimerBroken")
            permTimer("qaStopTimerMoving", "", 3600, "")
            enableTimer("qaStopTimerMoving")
            permKey("qaStopKeyFolder", "", -1, "")
            permKey("qaStopKeyOn", "qaStopKeyFolder", 16777268, "")
            permKey("qaStopKeyOff", "qaStopKeyFolder", 16777269, "")
            enableKey("qaStopKeyFolder")
            enableKey("qaStopKeyOn")
            disableKey("qaStopKeyOff")
            permKey("qaStopKeyBroken", "qaStopKeyFolder", 16777270, "")
            enableKey("qaStopKeyBroken")
        )")));

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
        mpEditor->rebuildStaleTrees();
    }

    void cleanup()
    {
        if (mpHost && mpHost->mEmergencyStop) {
            mpHost->reenableAllTriggers();
        }
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

    void test_itemsLeftDuringAStopKeepTheirLook_data() { addKinds(); }

    void test_itemsLeftDuringAStopKeepTheirLook()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        QTreeWidget* pTree = showView(kind);
        const QList<QTreeWidgetItem*> items = itemsOf(pTree, prefix);
        QVERIFY2(!items.contains(nullptr), "the items made for this test are not each in the tree once");
        QCOMPARE(lookOf(items.at(0)).description, mpEditor->descActiveFolder);
        QCOMPARE(lookOf(items.at(1)).description, mpEditor->descActive);
        QCOMPARE(lookOf(items.at(2)).description, mpEditor->descInactive);
        QList<Look> before;
        for (auto* pItem : items) {
            before << lookOf(pItem);
        }

        mpHost->stopAllTriggers();
        for (auto* pItem : items) {
            select(kind, pTree, pItem);
        }
        select(kind, pTree, items.at(0));

        for (int i = 0; i < items.count(); ++i) {
            QCOMPARE(lookOf(items.at(i)).description, before.at(i).description);
            QVERIFY2(lookOf(items.at(i)).iconKey == before.at(i).iconKey, qPrintable(qsl("\"%1\" changed its icon on being left").arg(items.at(i)->text(0))));
        }
        // saving an item it leaves must not let the item run before the stop ends
        QVERIFY2(!running(kind, items.at(1)), "leaving an item switched it back on during the emergency stop");

        mpHost->reenableAllTriggers();
        QVERIFY2(running(kind, items.at(1)), "ending the emergency stop did not switch the item back on");
        QVERIFY(!running(kind, items.at(2)));
    }

    void test_treesRebuiltDuringAStopShowWhatIsSwitchedOn_data() { addKinds(); }

    void test_treesRebuiltDuringAStopShowWhatIsSwitchedOn()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        mpEditor->rebuildStaleTrees();
        QTreeWidget* pTree = showView(kind);
        QList<Look> before;
        for (auto* pItem : itemsOf(pTree, prefix)) {
            QVERIFY2(pItem, "the items made for this test are not each in the tree once");
            before << lookOf(pItem);
        }
        QCOMPARE(before.at(0).description, mpEditor->descActiveFolder);
        QCOMPARE(before.at(1).description, mpEditor->descActive);
        QCOMPARE(before.at(2).description, mpEditor->descInactive);

        mpHost->stopAllTriggers();
        mpEditor->rebuildStaleTrees();
        pTree = showView(kind);

        const QList<QTreeWidgetItem*> items = itemsOf(pTree, prefix);
        QVERIFY2(!items.contains(nullptr), "the rebuilt tree does not hold each of the items once");
        for (int i = 0; i < items.count(); ++i) {
            QCOMPARE(lookOf(items.at(i)).description, before.at(i).description);
            QVERIFY2(lookOf(items.at(i)).iconKey == before.at(i).iconKey, qPrintable(qsl("\"%1\" has another icon in the rebuilt tree").arg(items.at(i)->text(0))));
        }
    }

    void test_itemsMovedDuringAStopKeepTheirLook_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("prefix");
        QTest::newRow("trigger") << qsl("trigger") << qsl("qaStopTrigger");
        QTest::newRow("timer") << qsl("timer") << qsl("qaStopTimer");
    }

    void test_itemsMovedDuringAStopKeepTheirLook()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        mpEditor->rebuildStaleTrees();
        auto* pTree = static_cast<TTreeWidget*>(showView(kind));
        QTreeWidgetItem* pFolder = itemNamed(pTree, prefix + qsl("Folder"));
        QTreeWidgetItem* pMoving = itemNamed(pTree, prefix + qsl("Moving"));
        QVERIFY2(pFolder && pMoving, "the items made for this test are not each in the tree once");
        QCOMPARE(lookOf(pFolder).description, mpEditor->descActiveFolder);
        QCOMPARE(lookOf(pMoving).description, mpEditor->descActive);
        // At the top level its look is that of a switched-on item in a switched-on folder
        const QImage folderBefore = imageOf(pFolder);
        const QImage movingBefore = imageOf(pMoving);
        QVERIFY(!folderBefore.isNull() && !movingBefore.isNull());

        mpHost->stopAllTriggers();
        moveOntoFolder(pTree, pMoving, pFolder);

        QCOMPARE(pMoving->parent(), pFolder);
        QVERIFY2(imageOf(pFolder) == folderBefore, "the folder moved into has another icon");
        QVERIFY2(imageOf(pMoving) == movingBefore, "the moved item has another icon");
    }

    void test_aScriptThatDoesNotCompileStillShowsAsBrokenDuringAStop_data() { addKinds(); }

    void test_aScriptThatDoesNotCompileStillShowsAsBrokenDuringAStop()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        QTreeWidget* pTree = showView(kind);
        QTreeWidgetItem* pBroken = itemNamed(pTree, prefix + qsl("Broken"));
        QTreeWidgetItem* pOther = itemNamed(pTree, prefix + qsl("On"));
        QVERIFY2(pBroken && pOther, "the items made for this test are not each in the tree once");

        mpHost->stopAllTriggers();
        select(kind, pTree, pBroken);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("qaBroken = ("));
        select(kind, pTree, pOther);
        QCOMPARE(lookOf(pBroken).description, mpEditor->descError);
    }
};

#include "EditorEmergencyStopTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorEmergencyStopTest)
