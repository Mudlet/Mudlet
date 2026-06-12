/***************************************************************************
 *   Copyright (C) 2026 by Andrew Johnson - andrew@johnson5.net            *
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

#include "TKeySequenceEdit.h"
#include "utils.h"

#include <QLineEdit>
#include <QSignalSpy>
#include <QVBoxLayout>
#include <QtTest/QtTest>

// Pins the accessibility behaviour that TKeySequenceEdit adds on top of the
// stock QKeySequenceEdit (#8873). Key events are sent to the inner QLineEdit
// because that is where keyboard focus lives via the focus proxy.
class TKeySequenceEditTest : public QObject
{
    Q_OBJECT

    static QLineEdit* innerLineEdit(TKeySequenceEdit& edit) { return edit.findChild<QLineEdit*>(); }

private slots:
    void focusIsRoutedToInnerLineEdit()
    {
        TKeySequenceEdit edit(QKeySequence(qsl("Ctrl+O")), qsl("Open file"));

        auto* lineEdit = innerLineEdit(edit);
        QVERIFY(lineEdit);
        QCOMPARE(edit.focusProxy(), lineEdit);
        QVERIFY(!lineEdit->focusProxy());
    }

    void accessibleNameIsSetOnWidgetAndLineEdit()
    {
        const QString label = qsl("Open file");
        TKeySequenceEdit edit(QKeySequence(), label);

        QCOMPARE(edit.accessibleName(), label);
        QCOMPARE(innerLineEdit(edit)->accessibleName(), label);
    }

    void accessibleDescriptionReflectsBinding()
    {
        const QKeySequence sequence(qsl("Ctrl+O"));
        TKeySequenceEdit edit(sequence, qsl("Open file"));

        const QString description = edit.accessibleDescription();
        QVERIFY(description.contains(sequence.toString(QKeySequence::NativeText)));
        QCOMPARE(innerLineEdit(edit)->accessibleDescription(), description);
    }

    void accessibleDescriptionReportsUnsetBinding()
    {
        TKeySequenceEdit edit(QKeySequence(), qsl("Open file"));

        QVERIFY(!edit.accessibleDescription().isEmpty());
        QCOMPARE(innerLineEdit(edit)->accessibleDescription(), edit.accessibleDescription());
    }

    void accessibleDescriptionUpdatesOnSequenceChange()
    {
        TKeySequenceEdit edit(QKeySequence(), qsl("Open file"));
        const QString unsetDescription = edit.accessibleDescription();

        const QKeySequence sequence(qsl("Alt+F4"));
        edit.setKeySequence(sequence);
        QVERIFY(edit.accessibleDescription().contains(sequence.toString(QKeySequence::NativeText)));
        QCOMPARE(innerLineEdit(edit)->accessibleDescription(), edit.accessibleDescription());

        edit.clear();
        QCOMPARE(edit.accessibleDescription(), unsetDescription);
    }

    void lineEditRejectsDirectEditing()
    {
        TKeySequenceEdit edit(QKeySequence(), qsl("Open file"));

        auto* lineEdit = innerLineEdit(edit);
        QVERIFY(lineEdit->isReadOnly());
        QCOMPARE(lineEdit->contextMenuPolicy(), Qt::NoContextMenu);
        QVERIFY(!lineEdit->testAttribute(Qt::WA_InputMethodEnabled));
    }

    void captureStillRecordsKeyCombination()
    {
        TKeySequenceEdit edit(QKeySequence(), qsl("Open file"));

        QTest::keyClick(innerLineEdit(edit), Qt::Key_P, Qt::ControlModifier);
        QCOMPARE(edit.keySequence(), QKeySequence(Qt::CTRL | Qt::Key_P));
    }

    void bareModifierPressDoesNotClearBinding()
    {
        const QKeySequence sequence(qsl("Ctrl+O"));
        TKeySequenceEdit edit(sequence, qsl("Open file"));
        auto* lineEdit = innerLineEdit(edit);

        QTest::keyPress(lineEdit, Qt::Key_Shift);
        QCOMPARE(edit.keySequence(), sequence);
        QTest::keyRelease(lineEdit, Qt::Key_Shift);
        QCOMPARE(edit.keySequence(), sequence);

        QTest::keyClick(lineEdit, Qt::Key_Control);
        QCOMPARE(edit.keySequence(), sequence);
        QTest::keyClick(lineEdit, Qt::Key_Alt);
        QCOMPARE(edit.keySequence(), sequence);
        QTest::keyClick(lineEdit, Qt::Key_Meta);
        QCOMPARE(edit.keySequence(), sequence);
    }

    void shiftTabTraversalDoesNotChangeBinding()
    {
        const QKeySequence sequence(qsl("Ctrl+O"));
        TKeySequenceEdit edit(sequence, qsl("Open file"));
        auto* lineEdit = innerLineEdit(edit);

        // Shift+Tab arrives from the platform as Backtab with the Shift
        // modifier still set:
        QTest::keyClick(lineEdit, Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(edit.keySequence(), sequence);

        QTest::keyClick(lineEdit, Qt::Key_Tab, Qt::ShiftModifier);
        QCOMPARE(edit.keySequence(), sequence);
    }

    // The traversal tests need real focus movement: the capture is committed
    // by the focus-out that the traversal causes, mirroring how the stock
    // widget commits in focusOutEvent() when Tab moves focus away.
    void shiftBacktabCommitsCaptureAndMovesFocusBackwards()
    {
        QWidget window;
        auto* layout = new QVBoxLayout(&window);
        auto* neighbour = new QLineEdit(&window);
        auto* edit = new TKeySequenceEdit(QKeySequence(), qsl("Open file"), &window);
        layout->addWidget(neighbour);
        layout->addWidget(edit);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));

        edit->setFocus();
        auto* lineEdit = edit->findChild<QLineEdit*>();
        QTRY_VERIFY(lineEdit->hasFocus());

        QSignalSpy spy(edit, &QKeySequenceEdit::editingFinished);
        const QKeySequence captured(Qt::CTRL | Qt::Key_P);
        QTest::keyClick(lineEdit, Qt::Key_P, Qt::ControlModifier);
        QCOMPARE(edit->keySequence(), captured);

        QTest::keyClick(lineEdit, Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(edit->keySequence(), captured);
        QCOMPARE(spy.count(), 1);
        QTRY_VERIFY(neighbour->hasFocus());
    }

    void plainTabCommitsCaptureAndMovesFocusForwards()
    {
        QWidget window;
        auto* layout = new QVBoxLayout(&window);
        auto* edit = new TKeySequenceEdit(QKeySequence(), qsl("Open file"), &window);
        auto* neighbour = new QLineEdit(&window);
        layout->addWidget(edit);
        layout->addWidget(neighbour);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));

        edit->setFocus();
        auto* lineEdit = edit->findChild<QLineEdit*>();
        QTRY_VERIFY(lineEdit->hasFocus());

        QSignalSpy spy(edit, &QKeySequenceEdit::editingFinished);
        const QKeySequence captured(Qt::CTRL | Qt::Key_P);
        QTest::keyClick(lineEdit, Qt::Key_P, Qt::ControlModifier);

        QTest::keyClick(lineEdit, Qt::Key_Tab);
        QCOMPARE(edit->keySequence(), captured);
        QCOMPARE(spy.count(), 1);
        QTRY_VERIFY(neighbour->hasFocus());
    }
};

#include "TKeySequenceEditTest.moc"
QTEST_MAIN(TKeySequenceEditTest)
