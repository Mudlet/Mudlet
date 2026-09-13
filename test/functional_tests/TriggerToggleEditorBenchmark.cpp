/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// Report-only benchmark: what a Lua enableTrigger()/disableTrigger() call costs
// on a profile with many triggers, with the script editor closed versus open.
// Prints METRIC lines; never registered with ctest.
//
//   QT_QPA_PLATFORM=offscreen ./TriggerToggleEditorBenchmark

#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>
#include <clocale>
#include <cstdio>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

using namespace std::chrono_literals;

class TriggerToggleEditorBenchmark : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("TriggerToggleBench-Profile");
    const QString mLocalhost = qsl("localhost");

    static constexpr int kPasses = 5;
    // Toggles per pass: each iteration disables then re-enables one trigger.
    static constexpr int kTogglesPerPass = 1000;

    static void emitMetric(const QString& name, double value)
    {
        std::printf("METRIC %s %.3f\n", qPrintable(name), value);
        std::fflush(stdout);
    }

    bool runLua(const QString& code) { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    int mFolders = 0;
    int mTotal = 0;

    // Adds `folders` more folders of `perFolder` leaf triggers each; leaves are
    // t_<n> and folders f_<k>, numbering on from whatever is already planted
    // (permanent triggers cannot be removed from Lua, so each size level grows
    // the previous one).
    void plantTriggers(int folders, int perFolder)
    {
        const QString script = qsl(R"(
            local n = %3
            for f = %4, %4 + %1 - 1 do
                permGroup("f_" .. f, "trigger")
                for i = 1, %2 do
                    n = n + 1
                    permRegexTrigger("t_" .. n, "f_" .. f, {"^benchmark line " .. n .. "$"}, "")
                end
            end
        )")
                                       .arg(folders)
                                       .arg(perFolder)
                                       .arg(mTotal)
                                       .arg(mFolders + 1);
        QVERIFY(runLua(script));
        mFolders += folders;
        mTotal += folders * perFolder;
        mpEditor->doCleanReset();
        QVERIFY(waitForTreeToHold(qsl("t_%1").arg(mTotal)));
    }

    // The tree widget is a private Ui member; the object name is what the .ui
    // file gives it, so this keeps the harness out of the editor's friend list.
    QTreeWidget* triggerTree() const { return mpEditor->findChild<QTreeWidget*>(qsl("treeWidget_triggers")); }

    bool waitForTreeToHold(const QString& name) const
    {
        return QTest::qWaitFor(
                [this, &name]() {
                    return !triggerTree()->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0).isEmpty();
                },
                30000ms);
    }

    // Best-of-kPasses seconds for kTogglesPerPass disable+enable pairs. Spread
    // mode strides through the first `span` leaf triggers with a prime step, so
    // the toggled items sit at every depth and position of the tree rather than
    // clustering at its start; "last" mode toggles t_<span> only, the item a
    // depth-first lookup finds last.
    double timeToggles(int span, bool spread, bool includeEventProcessing)
    {
        const QString script = qsl(R"(
            local span = %1
            for i = 1, %2 do
                local name = %3
                disableTrigger(name)
                enableTrigger(name)
            end
        )")
                                       .arg(span)
                                       .arg(kTogglesPerPass)
                                       .arg(spread ? qsl("\"t_\" .. (((i * 7919) % span) + 1)") : qsl("\"t_\" .. span"));
        double best = 1e30;
        for (int pass = 0; pass < kPasses; ++pass) {
            QElapsedTimer timer;
            timer.start();
            if (!runLua(script)) {
                return -1.0;
            }
            if (includeEventProcessing) {
                QCoreApplication::processEvents();
                QCoreApplication::sendPostedEvents();
            }
            best = std::min(best, timer.nsecsElapsed() / 1.0e9);
        }
        return best;
    }

    // enableTrigger() on triggers that are already enabled: what a script that
    // re-arms its triggers defensively every prompt pays for no state change.
    double timeRedundantEnables(int span)
    {
        const QString script = qsl(R"(
            local span = %1
            for i = 1, %2 do
                enableTrigger("t_" .. (((i * 7919) % span) + 1))
            end
        )")
                                       .arg(span)
                                       .arg(kTogglesPerPass);
        double best = 1e30;
        for (int pass = 0; pass < kPasses; ++pass) {
            QElapsedTimer timer;
            timer.start();
            if (!runLua(script)) {
                return -1.0;
            }
            best = std::min(best, timer.nsecsElapsed() / 1.0e9);
        }
        return best;
    }

    // One game line's worth of work: `batch` distinct triggers (or folders)
    // disabled and re-enabled, then the event loop turned so any deferred
    // repaint runs. Best-of-kBatchReps microseconds per line.
    static constexpr int kBatchReps = 40;
    double timeBatchPerLine(int span, int batch, bool folders)
    {
        const QString script = qsl(R"(
            local span = %1
            for i = 1, %2 do
                local name = "%3" .. (((i * 7919) % span) + 1)
                disableTrigger(name)
                enableTrigger(name)
            end
        )")
                                       .arg(span)
                                       .arg(batch)
                                       .arg(folders ? qsl("f_") : qsl("t_"));
        double best = 1e30;
        for (int rep = 0; rep < kBatchReps; ++rep) {
            QElapsedTimer timer;
            timer.start();
            if (!runLua(script)) {
                return -1.0;
            }
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents();
            best = std::min(best, timer.nsecsElapsed() / 1.0e6);
        }
        return best;
    }

    double timeFolderToggles(int folders)
    {
        const QString script = qsl(R"(
            for i = 1, %1 do
                local name = "f_" .. ((i % %2) + 1)
                disableTrigger(name)
                enableTrigger(name)
            end
        )")
                                       .arg(kTogglesPerPass / 10)
                                       .arg(folders);
        double best = 1e30;
        for (int pass = 0; pass < kPasses; ++pass) {
            QElapsedTimer timer;
            timer.start();
            if (!runLua(script)) {
                return -1.0;
            }
            best = std::min(best, timer.nsecsElapsed() / 1.0e9);
        }
        return best;
    }

    void report(const QString& label)
    {
        const int total = mTotal;
        const int folders = mFolders;
        auto us = [](double secs, int toggles) {
            return secs / toggles * 1.0e6;
        };

        mpEditor->hide();
        QCoreApplication::processEvents();
        QVERIFY(!mpEditor->isVisible());
        const double hidden = timeToggles(total, true, false);
        QVERIFY(hidden >= 0);
        emitMetric(qsl("%1_n%2_editor_closed_us_per_toggle").arg(label).arg(total), us(hidden, 2 * kTogglesPerPass));

        mpEditor->show();
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
        QVERIFY(mpEditor->isVisible());
        triggerTree()->collapseAll();
        QCoreApplication::processEvents();
        const double openCollapsed = timeToggles(total, true, false);
        QVERIFY(openCollapsed >= 0);
        emitMetric(qsl("%1_n%2_editor_open_collapsed_us_per_toggle").arg(label).arg(total), us(openCollapsed, 2 * kTogglesPerPass));

        triggerTree()->expandAll();
        QCoreApplication::processEvents();
        const double openExpanded = timeToggles(total, true, false);
        QVERIFY(openExpanded >= 0);
        emitMetric(qsl("%1_n%2_editor_open_expanded_us_per_toggle").arg(label).arg(total), us(openExpanded, 2 * kTogglesPerPass));

        const double openExpandedLast = timeToggles(total, false, false);
        QVERIFY(openExpandedLast >= 0);
        emitMetric(qsl("%1_n%2_editor_open_expanded_last_item_us_per_toggle").arg(label).arg(total), us(openExpandedLast, 2 * kTogglesPerPass));

        const double openExpandedPaint = timeToggles(total, true, true);
        QVERIFY(openExpandedPaint >= 0);
        emitMetric(qsl("%1_n%2_editor_open_expanded_plus_events_us_per_toggle").arg(label).arg(total), us(openExpandedPaint, 2 * kTogglesPerPass));

        const double redundant = timeRedundantEnables(total);
        QVERIFY(redundant >= 0);
        emitMetric(qsl("%1_n%2_editor_open_expanded_redundant_enable_us_per_call").arg(label).arg(total), us(redundant, kTogglesPerPass));

        // Folder toggles: each repaints the folder's whole subtree.
        const double folderToggle = timeFolderToggles(folders);
        QVERIFY(folderToggle >= 0);
        emitMetric(qsl("%1_n%2_editor_open_expanded_folder_us_per_toggle").arg(label).arg(total), us(folderToggle, 2 * (kTogglesPerPass / 10)));

        for (const int batch : {1, 20, 100}) {
            const double perLine = timeBatchPerLine(total, batch, false);
            QVERIFY(perLine >= 0);
            emitMetric(qsl("%1_n%2_editor_open_expanded_line_of_%3_toggles_us").arg(label).arg(total).arg(batch), perLine * 1.0e3);
        }
        const double folderLine = timeBatchPerLine(folders, 5, true);
        QVERIFY(folderLine >= 0);
        emitMetric(qsl("%1_n%2_editor_open_expanded_line_of_5_folder_toggles_us").arg(label).arg(total), folderLine * 1.0e3);
        mpEditor->hide();
        QCoreApplication::processEvents();
        const double closedLine = timeBatchPerLine(total, 20, false);
        QVERIFY(closedLine >= 0);
        emitMetric(qsl("%1_n%2_editor_closed_line_of_20_toggles_us").arg(label).arg(total), closedLine * 1.0e3);
        mpEditor->show();
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();

        // Same toggles with the editor on another view (trigger tree not on screen)
        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
        const double otherView = timeToggles(total, true, false);
        QVERIFY(otherView >= 0);
        emitMetric(qsl("%1_n%2_editor_open_scripts_view_us_per_toggle").arg(label).arg(total), us(otherView, 2 * kTogglesPerPass));
        mpEditor->slot_showTriggers();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        std::setlocale(LC_NUMERIC, "C");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY(mpServer->isListening());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(2000), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
    }

    void cleanupTestCase()
    {
        if (mpHost) {
            if (auto* pEditor = mpHost->mpEditorDialog.data()) {
                mpHost->mpEditorDialog = nullptr;
                delete pEditor;
            }
        }
        delete mpServer;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void bench_500()
    {
        plantTriggers(10, 50);
        report(qsl("bench"));
    }

    void bench_2000()
    {
        plantTriggers(30, 50);
        report(qsl("bench"));
    }

    void bench_5000()
    {
        plantTriggers(60, 50);
        report(qsl("bench"));
    }
};
#include "TriggerToggleEditorBenchmark.moc"
QTEST_MAIN(TriggerToggleEditorBenchmark)
