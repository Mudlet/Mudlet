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
 * Looking at a running timer in the script editor saves it when the selection
 * moves on, and that save must leave its countdown alone (#2510). Only the
 * editor's form gets there, which no spec can drive.
 */

#include <QTemporaryDir>
#include <QTimeEdit>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTimer.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "edbee/models/textdocument.h"
#include "edbee/texteditorwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TimerEditorCountdownTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    QTreeWidget* mpTimerTree = nullptr;
    const QString mProfileName = qsl("TimerEditorCountdown-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    TTimer* timerNamed(const QString& name) const { return mpHost->getTimerUnit()->findFirstTimer(name); }

    QTreeWidgetItem* itemNamed(const QString& name) const
    {
        const auto found = mpTimerTree->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return found.size() == 1 ? found.first() : nullptr;
    }

    // Long enough that no timer here fires during the test, however slow the build
    static constexpr int scmInterval = 600'000;

    static bool underWay(TTimer* pTimer) { return pTimer->getQTimer()->isActive() && pTimer->remainingTime() < scmInterval - 1000; }

    // What clicking the timer and then another item in the tree does
    void viewThenLeave(QTreeWidgetItem* pItem)
    {
        mpEditor->slot_timerSelected(pItem);
        mpEditor->slot_timerSelected(itemNamed(qsl("qaElsewhere")));
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

        // permTimer() makes its timer switched off
        QVERIFY(runLua(qsl(R"(
            permTimer("qaElsewhere", "", 3600, "")
            permTimer("qaCountdown", "", 600, "qaCountdownFired = true")
            enableTimer("qaCountdown")
            permTimer("qaRetimed", "", 600, "qaRetimedFired = true")
            enableTimer("qaRetimed")
            permTimer("qaRetimedQuietly", "", 600, "qaRetimedQuietlyFired = true")
            enableTimer("qaRetimedQuietly")
            permTimer("qaBroken", "", 600, "qaBrokenFired = true")
            enableTimer("qaBroken")
            permTimer("qaMoved", "", 600, "qaMovedFired = true")
            enableTimer("qaMoved")
            permTimer("qaOffsetParent", "", 3600, "")
            enableTimer("qaOffsetParent")
            permTimer("qaOffset", "qaOffsetParent", 600, "qaOffsetFired = true")
            enableTimer("qaOffset")
            permTimer("qaOffsetReordered", "qaOffsetParent", 600, "qaOffsetReorderedFired = true")
            enableTimer("qaOffsetReordered")
        )")));

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
        mpEditor->doCleanReset();
        mpEditor->slot_showTimers();
        mpTimerTree = mpEditor->findChild<QTreeWidget*>(qsl("treeWidget_timers"));
        QVERIFY(mpTimerTree);
        QVERIFY2(QTest::qWaitFor([this]() {
                     return itemNamed(qsl("qaElsewhere")) && itemNamed(qsl("qaOffset"));
                 }),
                 "the editor never listed the timers");

        QVERIFY(timerNamed(qsl("qaOffset"))->isOffsetTimer());

        // A countdown that has visibly run, so that a restart shows in what is left of it
        QTest::qWait(1500ms);
        for (const QString& name : {qsl("qaCountdown"), qsl("qaRetimed"), qsl("qaRetimedQuietly"), qsl("qaBroken"), qsl("qaOffset"), qsl("qaOffsetReordered")}) {
            QVERIFY2(timerNamed(name) && underWay(timerNamed(name)), qPrintable(qsl("%1 is not counting down to start with").arg(name)));
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

    void test_viewingARunningTimerKeepsItsCountdown()
    {
        TTimer* pTimer = timerNamed(qsl("qaCountdown"));
        QVERIFY(pTimer);

        viewThenLeave(itemNamed(qsl("qaCountdown")));

        QVERIFY2(pTimer->getQTimer()->isActive(), "viewing the timer stopped it");
        QVERIFY2(underWay(pTimer), qPrintable(qsl("viewing the timer restarted it: %1 ms left").arg(pTimer->remainingTime())));
        QCOMPARE(pTimer->getTime(), QTime(0, 10));
    }

    void test_viewingAnArmedOffsetTimerKeepsItsCountdown()
    {
        TTimer* pTimer = timerNamed(qsl("qaOffset"));
        QVERIFY(pTimer);

        viewThenLeave(itemNamed(qsl("qaOffset")));

        QVERIFY2(pTimer->getQTimer()->isActive(), "viewing the offset timer disarmed it");
        QVERIFY2(underWay(pTimer), qPrintable(qsl("viewing the offset timer restarted it: %1 ms left").arg(pTimer->remainingTime())));
    }

    // The time fields save themselves as they change
    void test_savingANewTimeStillRestartsTheCountdown()
    {
        TTimer* pTimer = timerNamed(qsl("qaRetimed"));
        QVERIFY(pTimer);

        mpEditor->slot_timerSelected(itemNamed(qsl("qaRetimed")));
        auto* pMinutes = mpEditor->findChild<QTimeEdit*>(qsl("timeEdit_timer_minutes"));
        QVERIFY(pMinutes);
        pMinutes->setTime(QTime(0, 20));
        mpEditor->slot_timerSelected(itemNamed(qsl("qaElsewhere")));

        QCOMPARE(pTimer->getTime(), QTime(0, 20));
        QCOMPARE(pTimer->getQTimer()->interval(), 2 * scmInterval);
        QVERIFY2(pTimer->getQTimer()->isActive(), "saving a new time left the timer stopped");
        // More than the old time ever had left
        QVERIFY2(pTimer->remainingTime() > scmInterval, qPrintable(qsl("saving a new time did not restart the timer: %1 ms left").arg(pTimer->remainingTime())));
    }

    // A new time the fields have not saved yet is left for the item's own save
    void test_savingANewTimeTheFieldsHeldBackStillRestartsTheCountdown()
    {
        TTimer* pTimer = timerNamed(qsl("qaRetimedQuietly"));
        QVERIFY(pTimer);

        mpEditor->slot_timerSelected(itemNamed(qsl("qaRetimedQuietly")));
        auto* pMinutes = mpEditor->findChild<QTimeEdit*>(qsl("timeEdit_timer_minutes"));
        QVERIFY(pMinutes);
        {
            const QSignalBlocker blocker(pMinutes);
            pMinutes->setTime(QTime(0, 20));
        }
        QCOMPARE(pTimer->getTime(), QTime(0, 10));
        mpEditor->slot_timerSelected(itemNamed(qsl("qaElsewhere")));

        QCOMPARE(pTimer->getTime(), QTime(0, 20));
        QCOMPARE(pTimer->getQTimer()->interval(), 2 * scmInterval);
        QVERIFY2(pTimer->getQTimer()->isActive(), "saving a new time left the timer stopped");
        // More than the old time ever had left
        QVERIFY2(pTimer->remainingTime() > scmInterval, qPrintable(qsl("saving a new time did not restart the timer: %1 ms left").arg(pTimer->remainingTime())));
    }

    void test_savingAScriptThatDoesNotCompileStillStopsTheTimer()
    {
        TTimer* pTimer = timerNamed(qsl("qaBroken"));
        QVERIFY(pTimer);
        QVERIFY(pTimer->getQTimer()->isActive());

        mpEditor->slot_timerSelected(itemNamed(qsl("qaBroken")));
        auto* pScriptEditor = mpEditor->findChild<edbee::TextEditorWidget*>(qsl("edbeeEditorWidget"));
        QVERIFY(pScriptEditor);
        pScriptEditor->textDocument()->setText(qsl("this is not ( Lua"));
        mpEditor->slot_timerSelected(itemNamed(qsl("qaElsewhere")));

        QVERIFY(!pTimer->state());
        QVERIFY2(!pTimer->getQTimer()->isActive(), "a timer whose script no longer compiles kept counting down");
    }

    // Its parent firing arms an offset timer, being dragged under it does not
    void test_aTimerMovedUnderAnotherWaitsForItToFire()
    {
        TTimer* pTimer = timerNamed(qsl("qaMoved"));
        TTimer* pParent = timerNamed(qsl("qaOffsetParent"));
        QVERIFY(pTimer && pParent);
        QVERIFY(pTimer->getQTimer()->isActive());

        mpEditor->slot_timerSelected(itemNamed(qsl("qaMoved")));
        mpHost->getTimerUnit()->reParentTimer(pTimer->getID(), 0, pParent->getID(), -1, -1);
        QVERIFY(pTimer->isOffsetTimer());
        mpEditor->slot_timerSelected(itemNamed(qsl("qaElsewhere")));

        QVERIFY2(!pTimer->getQTimer()->isActive(), "a timer moved under another started an offset countdown of its own");
        QVERIFY(pTimer->shouldBeActive());
    }

    void test_reorderingAnArmedOffsetTimerKeepsItArmed()
    {
        TTimer* pTimer = timerNamed(qsl("qaOffsetReordered"));
        TTimer* pParent = timerNamed(qsl("qaOffsetParent"));
        QVERIFY(pTimer && pParent);
        QVERIFY(pTimer->getQTimer()->isActive());

        mpHost->getTimerUnit()->reParentTimer(pTimer->getID(), pParent->getID(), pParent->getID(), 0, 0);

        QVERIFY2(pTimer->getQTimer()->isActive(), "moving an armed offset timer among its siblings disarmed it");
    }
};

#include "TimerEditorCountdownTest.moc"
MUDLET_GROUPED_TEST_MAIN(TimerEditorCountdownTest)
