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

// An emergency stop switches items off only at run time: the editor must keep showing
// what the player switched on, and saving must not switch an item back on before it ends

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

    static QString newKeyName() { return dlgTriggerEditor::tr("New key"); }

    static QStringList suffixes(const QString& prefix)
    {
        QStringList result{qsl("Folder"), qsl("On"), qsl("Off"), qsl("OffFolder"), qsl("OffFolderChild"), qsl("BadFolder"), qsl("BadFolderChild")};
        if (prefix == qsl("qaStopTrigger")) {
            result << qsl("Chain") << qsl("ChainChild");
        } else if (prefix == qsl("qaStopTimer")) {
            result << qsl("Offset");
        }
        return result;
    }

    static QStringList namesOf(const QString& prefix)
    {
        QStringList names;
        for (const QString& suffix : suffixes(prefix)) {
            names << prefix + suffix;
        }
        return names;
    }

    // As the tree shows them, in the order expectedLooks() describes
    QList<QTreeWidgetItem*> itemsOf(QTreeWidget* pTree, const QString& prefix)
    {
        QList<QTreeWidgetItem*> items;
        for (const QString& name : namesOf(prefix)) {
            items << itemNamed(pTree, name);
        }
        return items;
    }

    // Outside a stop, so that no test can pass by comparing two stopped looks
    QStringList expectedLooks(const QString& prefix) const
    {
        const QString underOffFolder = mpEditor->descInactiveParent.arg(mpEditor->descActive);
        QStringList looks{mpEditor->descActiveFolder, mpEditor->descActive, mpEditor->descInactive, mpEditor->descInactiveFolder, underOffFolder, mpEditor->descError, underOffFolder};
        if (prefix == qsl("qaStopTrigger")) {
            looks << mpEditor->descActiveFilterChain << mpEditor->descActive;
        } else if (prefix == qsl("qaStopTimer")) {
            looks << mpEditor->descActiveOffsetTimer;
        }
        return looks;
    }

    QString unexpectedLook(const QString& prefix, const QList<QTreeWidgetItem*>& items) const
    {
        const QStringList names = namesOf(prefix);
        for (int i = 0; i < items.count(); ++i) {
            if (!items.at(i)) {
                return qsl("\"%1\" is not in the tree once").arg(names.at(i));
            }
        }
        const QStringList looks = expectedLooks(prefix);
        for (int i = 0; i < items.count(); ++i) {
            if (lookOf(items.at(i)).description != looks.at(i)) {
                return qsl("\"%1\" reads \"%2\", not \"%3\"").arg(items.at(i)->text(0), lookOf(items.at(i)).description, looks.at(i));
            }
        }
        return {};
    }

    QString changedLook(const QList<QTreeWidgetItem*>& items, const QList<Look>& before) const
    {
        for (int i = 0; i < items.count(); ++i) {
            const Look now = lookOf(items.at(i));
            if (now.description != before.at(i).description || now.iconKey != before.at(i).iconKey) {
                return qsl("\"%1\" changed from \"%2\" to \"%3\"").arg(items.at(i)->text(0), before.at(i).description, now.description);
            }
        }
        return {};
    }

    static QList<Look> looksOf(const QList<QTreeWidgetItem*>& items)
    {
        QList<Look> looks;
        for (auto* pItem : items) {
            looks << lookOf(pItem);
        }
        return looks;
    }

    void toggle(const QString& kind)
    {
        if (kind == qsl("trigger")) {
            mpEditor->activeToggle_trigger();
        } else if (kind == qsl("alias")) {
            mpEditor->activeToggle_alias();
        } else if (kind == qsl("timer")) {
            mpEditor->activeToggle_timer();
        } else {
            mpEditor->activeToggle_key();
        }
    }

    bool breakScript(const QString& kind, const QString& name)
    {
        QTreeWidgetItem* pItem = itemNamed(showView(kind), name);
        if (!pItem) {
            return false;
        }
        const int id = pItem->data(0, Qt::UserRole).toInt();
        const QString script = qsl("qaBroken = (");
        if (kind == qsl("trigger")) {
            return !mpHost->getTriggerUnit()->getTrigger(id)->setScript(script);
        }
        if (kind == qsl("alias")) {
            return !mpHost->getAliasUnit()->getAlias(id)->setScript(script);
        }
        if (kind == qsl("timer")) {
            return !mpHost->getTimerUnit()->getTimer(id)->setScript(script);
        }
        return !mpHost->getKeyUnit()->getKey(id)->setScript(script);
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
            permSubstringTrigger("qaStopTriggerOffFolder", "", {}, "")
            permSubstringTrigger("qaStopTriggerOffFolderChild", "qaStopTriggerOffFolder", {"qaStopTriggerOffFolderChild"}, "")
            disableTrigger("qaStopTriggerOffFolder")
            permSubstringTrigger("qaStopTriggerBadFolder", "", {}, "")
            permSubstringTrigger("qaStopTriggerBadFolderChild", "qaStopTriggerBadFolder", {"qaStopTriggerBadFolderChild"}, "")
            permSubstringTrigger("qaStopTriggerChain", "qaStopTriggerFolder", {"qaStopTriggerChain"}, "")
            permSubstringTrigger("qaStopTriggerChainChild", "qaStopTriggerChain", {"qaStopTriggerChainChild"}, "")
            permAlias("qaStopAliasFolder", "", "", "")
            permAlias("qaStopAliasOn", "qaStopAliasFolder", "^qaStopAliasOn$", "")
            permAlias("qaStopAliasOff", "qaStopAliasFolder", "^qaStopAliasOff$", "")
            disableAlias("qaStopAliasOff")
            permAlias("qaStopAliasBroken", "qaStopAliasFolder", "^qaStopAliasBroken$", "")
            permAlias("qaStopAliasOffFolder", "", "", "")
            permAlias("qaStopAliasOffFolderChild", "qaStopAliasOffFolder", "^qaStopAliasOffFolderChild$", "")
            disableAlias("qaStopAliasOffFolder")
            permAlias("qaStopAliasBadFolder", "", "", "")
            permAlias("qaStopAliasBadFolderChild", "qaStopAliasBadFolder", "^qaStopAliasBadFolderChild$", "")
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
            permTimer("qaStopTimerMovingFolder", "", 0, "")
            enableTimer("qaStopTimerMovingFolder")
            permTimer("qaStopTimerOffFolder", "", 0, "")
            permTimer("qaStopTimerOffFolderChild", "qaStopTimerOffFolder", 3600, "")
            enableTimer("qaStopTimerOffFolderChild")
            disableTimer("qaStopTimerOffFolder")
            permTimer("qaStopTimerBadFolder", "", 0, "")
            permTimer("qaStopTimerBadFolderChild", "qaStopTimerBadFolder", 3600, "")
            enableTimer("qaStopTimerBadFolder")
            enableTimer("qaStopTimerBadFolderChild")
            permTimer("qaStopTimerOffset", "qaStopTimerOn", 10, "")
            enableTimer("qaStopTimerOffset")
            permKey("qaStopKeyFolder", "", -1, "")
            permKey("qaStopKeyOn", "qaStopKeyFolder", 16777268, "")
            permKey("qaStopKeyOff", "qaStopKeyFolder", 16777269, "")
            enableKey("qaStopKeyFolder")
            enableKey("qaStopKeyOn")
            disableKey("qaStopKeyOff")
            permKey("qaStopKeyBroken", "qaStopKeyFolder", 16777270, "")
            enableKey("qaStopKeyBroken")
            permKey("qaStopKeyOffFolder", "", -1, "")
            permKey("qaStopKeyOffFolderChild", "qaStopKeyOffFolder", 16777271, "")
            enableKey("qaStopKeyOffFolderChild")
            disableKey("qaStopKeyOffFolder")
            permKey("qaStopKeyBadFolder", "", -1, "")
            permKey("qaStopKeyBadFolderChild", "qaStopKeyBadFolder", 16777272, "")
            enableKey("qaStopKeyBadFolder")
            enableKey("qaStopKeyBadFolderChild")
        )")));
        // saveKey() switches a key on only while it still has the name a new one is given
        QVERIFY(runLua(qsl(R"(permKey("%1", "qaStopKeyFolder", 16777273, "") enableKey("%1"))").arg(newKeyName())));

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
        for (const QString& kind : {qsl("trigger"), qsl("alias"), qsl("timer"), qsl("key")}) {
            mpEditor->rebuildStaleTrees();
            const QString prefix = qsl("qaStop%1%2").arg(kind.at(0).toUpper(), kind.mid(1));
            QVERIFY2(breakScript(kind, prefix + qsl("BadFolder")), qPrintable(qsl("could not break the script of %1BadFolder").arg(prefix)));
        }
        mpEditor->rebuildStaleTrees();
        // A trigger's first save draws even a filter chain as a plain trigger, stop or not
        QTreeWidgetItem* pChain = itemNamed(showView(qsl("trigger")), qsl("qaStopTriggerChain"));
        QVERIFY(pChain);
        mpHost->getTriggerUnit()->getTrigger(pChain->data(0, Qt::UserRole).toInt())->unmarkAsNew();
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
        QList<QTreeWidgetItem*> items = itemsOf(pTree, prefix);
        const QString unexpected = unexpectedLook(prefix, items);
        QVERIFY2(unexpected.isEmpty(), qPrintable(unexpected));
        // Saving renames it after its binding, so only this test can find it
        QTreeWidgetItem* pNewKey = kind == qsl("key") ? itemNamed(pTree, newKeyName()) : nullptr;
        if (kind == qsl("key")) {
            QVERIFY2(pNewKey, "the new key is not in the tree once");
            QCOMPARE(lookOf(pNewKey).description, mpEditor->descActive);
            items << pNewKey;
        }
        const QList<Look> before = looksOf(items);

        mpHost->stopAllTriggers();
        // Twice, as only an item's first save takes the branch for new items
        for (int pass = 0; pass < 2; ++pass) {
            for (auto* pItem : items) {
                // Saving its broken script would switch a trigger off for good
                if (!pItem->text(0).endsWith(qsl("BadFolder"))) {
                    select(kind, pTree, pItem);
                }
            }
            select(kind, pTree, items.at(0));
            const QString changed = changedLook(items, before);
            QVERIFY2(changed.isEmpty(), qPrintable(qsl("on leaving it (pass %1): %2").arg(pass + 1).arg(changed)));
        }
        // saving an item it leaves must not let the item run before the stop ends
        QVERIFY2(!running(kind, items.at(1)), "leaving an item switched it back on during the emergency stop");
        QVERIFY2(!pNewKey || !running(kind, pNewKey), "leaving a new key switched it on during the emergency stop");

        mpHost->reenableAllTriggers();
        QVERIFY2(running(kind, items.at(1)), "ending the emergency stop did not switch the item back on");
        QVERIFY2(!pNewKey || running(kind, pNewKey), "ending the emergency stop did not switch the new key on");
        QVERIFY(!running(kind, items.at(2)));
    }

    void test_treesRebuiltDuringAStopShowWhatIsSwitchedOn_data() { addKinds(); }

    void test_treesRebuiltDuringAStopShowWhatIsSwitchedOn()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        mpEditor->rebuildStaleTrees();
        QTreeWidget* pTree = showView(kind);
        const QList<QTreeWidgetItem*> itemsBefore = itemsOf(pTree, prefix);
        const QString unexpected = unexpectedLook(prefix, itemsBefore);
        QVERIFY2(unexpected.isEmpty(), qPrintable(unexpected));
        const QList<Look> before = looksOf(itemsBefore);

        mpHost->stopAllTriggers();
        mpEditor->rebuildStaleTrees();
        pTree = showView(kind);

        const QList<QTreeWidgetItem*> items = itemsOf(pTree, prefix);
        QVERIFY2(!items.contains(nullptr), qPrintable(qsl("in the rebuilt tree: %1").arg(unexpectedLook(prefix, items))));
        const QString changed = changedLook(items, before);
        QVERIFY2(changed.isEmpty(), qPrintable(qsl("in the rebuilt tree: %1").arg(changed)));
    }

    void test_itemsToggledDuringAStopKeepTheirLook_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("prefix");
        QTest::addColumn<QString>("toggled");
        // A top-level folder reads the same either way once toggled, so each kind also toggles an item in a stopped folder
        QTest::newRow("trigger folder") << qsl("trigger") << qsl("qaStopTrigger") << qsl("Folder");
        QTest::newRow("trigger chain") << qsl("trigger") << qsl("qaStopTrigger") << qsl("Chain");
        QTest::newRow("alias folder") << qsl("alias") << qsl("qaStopAlias") << qsl("Folder");
        QTest::newRow("alias") << qsl("alias") << qsl("qaStopAlias") << qsl("On");
        QTest::newRow("timer folder") << qsl("timer") << qsl("qaStopTimer") << qsl("Folder");
        QTest::newRow("timer with an offset timer") << qsl("timer") << qsl("qaStopTimer") << qsl("On");
        QTest::newRow("key folder") << qsl("key") << qsl("qaStopKey") << qsl("Folder");
        QTest::newRow("key") << qsl("key") << qsl("qaStopKey") << qsl("On");
    }

    void test_itemsToggledDuringAStopKeepTheirLook()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        QFETCH(QString, toggled);
        mpEditor->rebuildStaleTrees();
        QTreeWidget* pTree = showView(kind);
        const QList<QTreeWidgetItem*> items = itemsOf(pTree, prefix);
        const QString unexpected = unexpectedLook(prefix, items);
        QVERIFY2(unexpected.isEmpty(), qPrintable(unexpected));
        const QList<Look> before = looksOf(items);

        mpHost->stopAllTriggers();
        select(kind, pTree, itemNamed(pTree, prefix + toggled));
        toggle(kind);
        toggle(kind);

        const QString changed = changedLook(items, before);
        QVERIFY2(changed.isEmpty(), qPrintable(qsl("after switching %1%2 off and on: %3").arg(prefix, toggled, changed)));
    }

    void test_itemsMovedDuringAStopKeepTheirLook_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("prefix");
        QTest::addColumn<QString>("moving");
        QTest::addColumn<QString>("look");
        QTest::newRow("trigger") << qsl("trigger") << qsl("qaStopTrigger") << qsl("qaStopTriggerMoving") << qsl("active");
        QTest::newRow("timer") << qsl("timer") << qsl("qaStopTimer") << qsl("qaStopTimerMoving") << qsl("active");
        QTest::newRow("timer folder") << qsl("timer") << qsl("qaStopTimer") << qsl("qaStopTimerMovingFolder") << qsl("activeFolder");
    }

    void test_itemsMovedDuringAStopKeepTheirLook()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        QFETCH(QString, moving);
        QFETCH(QString, look);
        mpEditor->rebuildStaleTrees();
        auto* pTree = static_cast<TTreeWidget*>(showView(kind));
        QTreeWidgetItem* pFolder = itemNamed(pTree, prefix + qsl("Folder"));
        QTreeWidgetItem* pMoving = itemNamed(pTree, moving);
        QVERIFY2(pFolder && pMoving, "the items made for this test are not each in the tree once");
        QCOMPARE(lookOf(pFolder).description, mpEditor->descActiveFolder);
        QCOMPARE(lookOf(pMoving).description, look == qsl("active") ? mpEditor->descActive : mpEditor->descActiveFolder);
        // At the top level its look is that of a switched-on item in a switched-on folder
        const QImage folderBefore = imageOf(pFolder);
        const QImage movingBefore = imageOf(pMoving);
        QVERIFY(!folderBefore.isNull() && !movingBefore.isNull());

        mpHost->stopAllTriggers();
        moveOntoFolder(pTree, pMoving, pFolder);

        QCOMPARE(pMoving->parent(), pFolder);
        // Only a trigger's move repaints the folder it lands in
        if (kind == qsl("trigger")) {
            QVERIFY2(imageOf(pFolder) == folderBefore, "the folder moved into has another icon");
        }
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
        const qint64 errorIcon = mpEditor->cachedIcon(qsl(":/icons/tools-report-bug.png")).cacheKey();
        QCOMPARE(lookOf(pBroken).description, mpEditor->descError);
        QCOMPARE(lookOf(pBroken).iconKey, errorIcon);

        mpEditor->rebuildStaleTrees();
        pBroken = itemNamed(showView(kind), prefix + qsl("Broken"));
        QVERIFY2(pBroken, "the rebuilt tree does not hold the item once");
        QCOMPARE(lookOf(pBroken).description, mpEditor->descError);
        QCOMPARE(lookOf(pBroken).iconKey, errorIcon);
    }
};

#include "EditorEmergencyStopTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorEmergencyStopTest)
