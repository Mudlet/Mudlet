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
 * Tab with the caret at offset zero crashed: edbee's TabCommand stepped its size_t
 * offset back before checking, wrapping to npos and reading charAt() out of bounds.
 * A new, empty item opens with the caret there.
 *
 * Run with: ctest -R EditorTabAtStartTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorTabAtStartTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorTabAtStart-Test-Profile");

    edbee::TextEditorController* controller() const { return mpEditor->mpSourceEditorEdbee->controller(); }

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
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        QTest::qWait(50ms);
        QVERIFY2(controller() != nullptr, "the code pane has no controller to type into");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_tabAtTheStartOfAnEmptyDocumentDoesNotTakeTheClientDown()
    {
        controller()->textDocument()->setText(QString());
        controller()->moveCaretToOffset(0, false);
        QCOMPARE(controller()->textSelection()->range(0).caret(), static_cast<size_t>(0));

        // The command edbee binds Tab to: the crash was in it, not in the key handling
        controller()->executeCommand(qsl("tab"));

        qInfo().noquote() << qsl("  after Tab the document holds %1 characters").arg(QString::number(controller()->textDocument()->length()));
        QVERIFY2(controller()->textDocument()->length() > 0, "Tab at the start of an empty document inserted nothing at all");
    }

    void test_tabAtTheStartOfALineOfTextDoesNotTakeTheClientDown()
    {
        controller()->textDocument()->setText(qsl("local a = 1\n"));
        controller()->moveCaretToOffset(0, false);
        QCOMPARE(controller()->textSelection()->range(0).caret(), static_cast<size_t>(0));

        controller()->executeCommand(qsl("tab"));

        const QString after = controller()->textDocument()->text();
        qInfo().noquote() << qsl("  the first line is now \"%1\"").arg(after.section(QChar::LineFeed, 0, 0));
        QVERIFY2(after.contains(qsl("local a = 1")), "Tab at the start of the line lost the line it was on");
    }
};

#include "EditorTabAtStartTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorTabAtStartTest)
