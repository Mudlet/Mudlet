/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * The sidebar's first row, the search field and the edit column's lead (notice or
 * name row) start at one y. The columns are compared with each other, not with
 * scmEditorColumnTopInset, so a column that stops using the inset fails.
 *
 * Run with: ctest -R EditorColumnAlignmentTest -V
 */

#include <QComboBox>
#include <QFrame>
#include <QListWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorColumnAlignmentTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorColumnAlignment-Test-Profile");

    // The window is the one frame all three columns share; each is inset from a different parent
    int topEdgeOf(QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).y(); }

    // The first row's pill, not the list widget carrying it, is where the reader sees the column start
    int sidebarFirstRowTop() const
    {
        QListWidget* pSidebar = mpEditor->mpListWidget_editorSidebar;
        if (!pSidebar || pSidebar->count() < 1) {
            return -1;
        }
        return pSidebar->viewport()->mapTo(mpEditor, pSidebar->visualItemRect(pSidebar->item(0)).topLeft()).y();
    }

    int searchFieldTop() const { return topEdgeOf(mpEditor->comboBox_searchTerms); }

    int noticeTop() const { return topEdgeOf(mpEditor->mpSystemMessageArea->frame_notificationArea); }

    int nameRowTop() const { return topEdgeOf(mpEditor->mpTriggersMainArea->widget_top); }

    static QString drift(const QString& what, const int measured, const int sidebar) { return qsl("%1 starts at %2 and the sidebar's first row at %3").arg(what).arg(measured).arg(sidebar); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1000, 800);

        // Something to edit, so the column shows the trigger form rather than an empty page
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theSearchFieldStartsWhereTheSidebarDoes()
    {
        mpEditor->hideSystemMessageArea();
        QTest::qWait(50ms);

        const int sidebar = sidebarFirstRowTop();
        QVERIFY2(sidebar > 0, "the sidebar has no first row to measure");
        qInfo().noquote() << qsl("sidebar %1, search field %2, name row %3").arg(sidebar).arg(searchFieldTop()).arg(nameRowTop());

        QVERIFY2(searchFieldTop() == sidebar, qPrintable(drift(qsl("the search field"), searchFieldTop(), sidebar)));
    }

    void test_theNameRowStartsWhereTheSidebarDoesWithNoNotice()
    {
        mpEditor->hideSystemMessageArea();
        QTest::qWait(50ms);
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), "the notice is still showing, so the name row is not what leads the column");

        const int sidebar = sidebarFirstRowTop();
        QVERIFY2(nameRowTop() == sidebar, qPrintable(drift(qsl("the name row"), nameRowTop(), sidebar)));
    }

    void test_theNoticeStartsWhereTheSidebarDoes()
    {
        mpEditor->showInfo(qsl("A notice, so that the column an item is edited in is led by one"));
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice did not come up, so there is nothing to measure");

        const int sidebar = sidebarFirstRowTop();
        qInfo().noquote() << qsl("sidebar %1, search field %2, notice %3").arg(sidebar).arg(searchFieldTop()).arg(noticeTop());

        QVERIFY2(noticeTop() == sidebar, qPrintable(drift(qsl("the notice"), noticeTop(), sidebar)));
        QVERIFY2(searchFieldTop() == sidebar, qPrintable(drift(qsl("the search field"), searchFieldTop(), sidebar)));

        mpEditor->hideSystemMessageArea();
        QTest::qWait(50ms);
    }
};

#include "EditorColumnAlignmentTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorColumnAlignmentTest)
