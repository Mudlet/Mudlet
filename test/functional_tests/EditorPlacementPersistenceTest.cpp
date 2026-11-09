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
 * The editor must reopen where it was left, also after a hide and show: the
 * show slots re-centred the hidden singleton. Driven through those slots, since
 * a test that constructs the editor itself never runs them.
 */

#include <QGuiApplication>
#include <QScreen>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorPlacementPersistenceTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorPlacementPersistence-Test-Profile");

    QPoint mSeededPos;
    QPoint mDraggedPos;
    QSize mSeededSize;

    static QString describe(const QPoint& point) { return qsl("(%1,%2)").arg(QString::number(point.x()), QString::number(point.y())); }

    void report(const char* step) const { qInfo().noquote() << qsl("  %1: pos %2 size %3").arg(QString::fromLatin1(step), describe(mpEditor->pos()), TestPixels::describe(mpEditor->size())); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));

        const QScreen* pScreen = QGuiApplication::primaryScreen();
        QVERIFY(pScreen);
        const QRect availableArea = pScreen->availableGeometry();

        // Inside the desktop, so showEvent() has no reason to move the editor, and
        // far enough apart and off-centre that no rounding confuses the two
        mSeededPos = availableArea.topLeft() + QPoint(24, 18);
        mDraggedPos = availableArea.topLeft() + QPoint(103, 71);
        mSeededSize = QSize(640, 460).boundedTo(availableArea.size());
        QVERIFY2(mSeededSize.height() >= 300, "The screen is too small for this test to say anything");

        MudletApp::getQSettings()->setValue(qsl("script_editor_pos"), mSeededPos);
        MudletApp::getQSettings()->setValue(qsl("script_editor_size"), mSeededSize);

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();
        qInfo().noquote() << qsl("  seeded: pos %1 size %2").arg(describe(mSeededPos), TestPixels::describe(mSeededSize));
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_editorOpensAtTheStoredPosition()
    {
        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(100ms);

        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        report("after first show");

        QVERIFY2(mpEditor->pos() == mSeededPos, qPrintable(qsl("The editor was stored at %1 but opened at %2 - showing it moved it").arg(describe(mSeededPos), describe(mpEditor->pos()))));
    }

    // Size is asserted too, so position and size are held to the same behaviour
    void test_hidingAndShowingKeepsThePlacement()
    {
        // The move stands in for a drag: moveEvent() cannot tell the two apart,
        // and it is what marks the placement as the user's own
        mpEditor->move(mDraggedPos);
        QCoreApplication::processEvents();
        const QSize sizeBefore = mpEditor->size();

        mpEditor->hide();
        QCoreApplication::processEvents();
        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(100ms);
        report("after hide and show");

        QVERIFY2(mpEditor->pos() == mDraggedPos, qPrintable(qsl("The editor was left at %1 and came back at %2 - it is moved on every show").arg(describe(mDraggedPos), describe(mpEditor->pos()))));
        QVERIFY2(mpEditor->size() == sizeBefore, qPrintable(qsl("The editor was left at %1 and came back at %2").arg(TestPixels::describe(sizeBefore), TestPixels::describe(mpEditor->size()))));
    }

    // positionDialogOnParentScreen() repositions any dialog that is not visible,
    // and the editor is hidden when this slot runs
    void test_theTriggerSlotKeepsThePlacementToo()
    {
        mpEditor->move(mDraggedPos);
        QCoreApplication::processEvents();
        mpEditor->hide();
        QCoreApplication::processEvents();
        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        report("after hide and show via the trigger slot");

        QVERIFY2(mpEditor->pos() == mDraggedPos, qPrintable(qsl("The editor was left at %1 and the trigger slot brought it back at %2").arg(describe(mDraggedPos), describe(mpEditor->pos()))));
    }

    void test_thePlacementSurvivesAFreshEditor()
    {
        const QSize sizeBefore = mpEditor->size();

        mpEditor->close();
        QCoreApplication::processEvents();

        const QPoint storedPos = MudletApp::getQSettings()->value(qsl("script_editor_pos")).toPoint();
        const QSize storedSize = MudletApp::getQSettings()->value(qsl("script_editor_size")).toSize();
        qInfo().noquote() << qsl("  in QSettings on close: pos %1 size %2").arg(describe(storedPos), TestPixels::describe(storedSize));
        QVERIFY2(storedPos == mDraggedPos, qPrintable(qsl("The editor was at %1 when it closed but stored %2").arg(describe(mDraggedPos), describe(storedPos))));
        QCOMPARE(storedSize, sizeBefore);

        delete mpEditor;
        mpEditor = nullptr;
        QVERIFY2(HostDialogs::of(mpHost).mpEditorDialog.isNull(), "The editor should have been destroyed");

        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "A fresh editor should have been created");
        report("after a fresh construction");

        QVERIFY2(mpEditor->pos() == mDraggedPos, qPrintable(qsl("A fresh editor should have opened at the stored %1, it opened at %2").arg(describe(mDraggedPos), describe(mpEditor->pos()))));
        QCOMPARE(mpEditor->size(), sizeBefore);
    }
};

#include "EditorPlacementPersistenceTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorPlacementPersistenceTest)
