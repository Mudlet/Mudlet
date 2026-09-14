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
 * How much of a trigger's pattern list the editor shows, and which row it
 * opens on.
 *
 * With the advanced options collapsed the pattern list used to be given the
 * height of one and a half rows, so a three pattern trigger opened with its
 * second and third patterns behind a scrollbar - on the very screens the
 * collapse was there to make room on. On top of that the list scrolled to the
 * row after the last pattern on selection, which put pattern 1 out of sight.
 *
 * Run with: ctest -R TriggerPatternListLayoutTest -V
 */

#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

class TriggerPatternListLayoutTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("TriggerPatternListLayout-Test");
    const QString mLocalhost = qsl("localhost");
    const QString mThreePatternTrigger = qsl("qaThreePatterns");
    const QString mNinePatternTrigger = qsl("qaNinePatterns");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void populateProfile()
    {
        auto* pThree = new TTrigger(nullptr, mpHost);
        pThree->setName(mThreePatternTrigger);
        pThree->setRegexCodeList({qsl("alpha"), qsl("beta"), qsl("gamma")}, {REGEX_PERL, REGEX_PERL, REGEX_PERL});
        pThree->setScript(qsl("echo(\"hi\")\n"));
        QVERIFY(pThree->registerTrigger());

        QStringList patterns;
        QList<int> kinds;
        for (int i = 1; i <= 9; ++i) {
            patterns << qsl("p%1").arg(i);
            kinds << REGEX_PERL;
        }
        auto* pNine = new TTrigger(nullptr, mpHost);
        pNine->setName(mNinePatternTrigger);
        pNine->setRegexCodeList(patterns, kinds);
        pNine->setScript(qsl("echo(\"hi\")\n"));
        QVERIFY(pNine->registerTrigger());
    }

    QTreeWidgetItem* triggerItem(const QString& name) const
    {
        const auto items = mpEditor->treeWidget_triggers->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return items.isEmpty() ? nullptr : items.first();
    }

    // A click on the tree is what a user does; the tree's own signal is what
    // carries that into the editor, so calling the slot is the same journey
    // without needing the item to be under a synthetic cursor
    bool selectTrigger(const QString& name)
    {
        auto* pItem = triggerItem(name);
        if (!pItem) {
            return false;
        }
        mpEditor->treeWidget_triggers->setCurrentItem(pItem);
        mpEditor->slot_triggerSelected(pItem);
        QCoreApplication::processEvents();
        return true;
    }

    // Rows that a user can read and type into in full - a row clipped by the
    // viewport edge is the "half row" the bug was about, so it does not count
    int fullyVisiblePatternRows() const
    {
        auto* viewport = mpEditor->mpScrollArea->viewport();
        const QRect visible = viewport->rect();
        int count = 0;
        for (int i = 0; i < mpEditor->mVisiblePatternCount && i < mpEditor->mTriggerPatternEdit.size(); ++i) {
            auto* pRow = mpEditor->mTriggerPatternEdit.at(i);
            if (!pRow || !pRow->isVisible()) {
                continue;
            }
            const QRect rowRect(pRow->mapTo(viewport, QPoint(0, 0)), pRow->size());
            if (visible.contains(rowRect)) {
                ++count;
            }
        }
        return count;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
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
        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        // The editor filled its trees when the profile loaded, so items added
        // behind its back are only in them after the same rebuild that Host
        // does when a package is installed
        mpEditor->doCleanReset();
        QVERIFY2(QTest::qWaitFor([this]() {
                     return triggerItem(mThreePatternTrigger) != nullptr && triggerItem(mNinePatternTrigger) != nullptr;
                 }),
                 "the editor never rebuilt its trees around the triggers this test planted");

        // The size the issue was reported at. It is the small-screen end of
        // what Mudlet is used on, which is exactly where the advanced options
        // get collapsed to buy the pattern list room.
        mpEditor->resize(1100, 700);
        QVERIFY(QTest::qWaitForWindowExposed(mpEditor));
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()
    {
        // ~Host would do this, but only if the host is ever destroyed - deleting
        // the editor here keeps the leak checker satisfied either way
        if (mpHost && mpHost->mpEditorDialog) {
            mpHost->mpEditorDialog->deleteLater();
            mpHost->mpEditorDialog = nullptr;
        }
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        // Every test here is about the collapsed state - expanding the advanced
        // options gives the main area enough height that the list is fine
        mpEditor->slot_showAllTriggerControls(false);
        QCoreApplication::processEvents();
    }

    // A three pattern trigger draws four rows: its patterns and the blank one
    // waiting for a fourth. All four have to be readable without scrolling.
    void test_collapsedAdvancedOptionsShowsAWholeSmallTrigger()
    {
        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QVERIFY2(!mpEditor->mpTriggersMainArea->toolButton_toggleExtraControls->isChecked(), "this test is about the collapsed state and the advanced options are showing");

        QCOMPARE(mpEditor->mVisiblePatternCount, 4);
        const int shown = fullyVisiblePatternRows();
        QVERIFY2(shown >= 4,
                 qPrintable(qsl("all 4 rows of a three pattern trigger should be readable at 1100x700 with the advanced options collapsed, but only %1 of them are - the "
                                "pattern list viewport is %2 pixels tall and a row is %3")
                                    .arg(QString::number(shown), QString::number(mpEditor->mpScrollArea->viewport()->height()), QString::number(mpEditor->mTriggerPatternEdit.at(0)->height()))));
        QCOMPARE(mpEditor->mpScrollArea->verticalScrollBar()->maximum(), 0);
    }

    // However many patterns a trigger has, the list is never squeezed down to
    // the row and a half that started this
    void test_aLongPatternListStillShowsSeveralRows()
    {
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not select the nine pattern trigger");

        const int shown = fullyVisiblePatternRows();
        QVERIFY2(shown >= dlgTriggerEditor::csmMinimumVisiblePatternRows,
                 qPrintable(qsl("the pattern list should always have room for %1 rows, but shows %2").arg(QString::number(dlgTriggerEditor::csmMinimumVisiblePatternRows), QString::number(shown))));
    }

    // Selecting a trigger is the moment a user goes looking for pattern 1, so
    // that is the row the list opens on - whatever it was scrolled to before
    void test_theListOpensOnPatternOne()
    {
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not select the nine pattern trigger");
        auto* pScrollBar = mpEditor->mpScrollArea->verticalScrollBar();
        QVERIFY2(pScrollBar->maximum() > 0, "the nine pattern trigger should overflow the list, otherwise this test cannot tell a reset scroll position from no scrolling at all");

        // Leave it at the bottom - where scrolling to the row after the last
        // pattern used to leave it, and where a user may leave it by hand
        pScrollBar->setValue(pScrollBar->maximum());
        QCoreApplication::processEvents();
        QVERIFY(pScrollBar->value() > 0);

        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not re-select the nine pattern trigger");

        QCOMPARE(pScrollBar->value(), 0);
        auto* viewport = mpEditor->mpScrollArea->viewport();
        auto* pFirstRow = mpEditor->mTriggerPatternEdit.at(0);
        const QRect firstRowRect(pFirstRow->mapTo(viewport, QPoint(0, 0)), pFirstRow->size());
        QVERIFY2(viewport->rect().contains(firstRowRect), "pattern 1 should be fully in view when a trigger is opened");
    }
};

#include "TriggerPatternListLayoutTest.moc"
MUDLET_GROUPED_TEST_MAIN(TriggerPatternListLayoutTest)
