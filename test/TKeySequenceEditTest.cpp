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

#include <QApplication>
#include <QLineEdit>
#include <QSignalSpy>
#include <QVBoxLayout>
#include <QtTest/QtTest>

static constexpr const char* activationUnavailableMessage = "the window never became active, so focus traversal cannot be exercised - this "
                                                            "display has no window manager. Run the suite through ctest, or set "
                                                            "QT_QPA_PLATFORM=offscreen, or start a window manager such as openbox.";

// Shared by the two traversal cases, because QSKIP and QFAIL only work from the
// test function itself and the two must not drift apart. ctest sets
// MUDLET_REQUIRE_WINDOW_ACTIVATION because every display it runs against can
// activate a window, so a skip there would be hiding a regression rather than
// reporting an environment - the same floor MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK
// puts under the media tests. qWaitForWindowActive() only polls, so a transient
// denial of activation timed the wait out on the macOS CI leg (#10116) - keep
// asking, as DetachedWindowMenuShortcutsTest already does.
#define REQUIRE_WINDOW_ACTIVATION(window)                                                                                                                                                              \
    do {                                                                                                                                                                                               \
        if (!QTest::qWaitFor([&]() {                                                                                                                                                                   \
                (window).activateWindow();                                                                                                                                                             \
                return QApplication::activeWindow() == &(window);                                                                                                                                      \
            })) {                                                                                                                                                                                      \
            if (qEnvironmentVariableIsSet("MUDLET_REQUIRE_WINDOW_ACTIVATION")) {                                                                                                                       \
                QFAIL(activationUnavailableMessage);                                                                                                                                                   \
            }                                                                                                                                                                                          \
            QSKIP(activationUnavailableMessage);                                                                                                                                                       \
        }                                                                                                                                                                                              \
    } while (false)

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

    void accessibleNameIsOnInnerFieldOnly()
    {
        const QString label = qsl("Open file");
        TKeySequenceEdit edit(QKeySequence(), label);

        // The inner field carries the accessible name; the wrapper is left
        // unnamed so a screen reader does not announce the label twice - once
        // for the wrapper's grouping and once for the focused field (#9322):
        QCOMPARE(innerLineEdit(edit)->accessibleName(), label);
        QVERIFY(edit.accessibleName().isEmpty());
    }

    void populatedBindingIsAnnouncedOnlyAsTheFieldValue()
    {
        const QKeySequence sequence(qsl("Ctrl+O"));
        TKeySequenceEdit edit(sequence, qsl("Open file"));
        auto* lineEdit = innerLineEdit(edit);

        // The binding is shown as the field's text, which screen readers
        // announce as its value, so it must not also appear in the accessible
        // description of the field or the wrapper - otherwise the reader speaks
        // the combination several times over (#9322):
        QCOMPARE(edit.keySequence(), sequence);
        QVERIFY(!lineEdit->text().isEmpty());
        QVERIFY(lineEdit->accessibleDescription().isEmpty());
        QVERIFY(edit.accessibleDescription().isEmpty());
    }

    void emptyBindingIsDescribedAsUnset()
    {
        TKeySequenceEdit edit(QKeySequence(), qsl("Open file"));
        auto* lineEdit = innerLineEdit(edit);

        // With no value to announce, the empty state is conveyed by the field's
        // description instead - but still only once: the wrapper never carries
        // it (#9322):
        QVERIFY(!lineEdit->accessibleDescription().isEmpty());
        QVERIFY(edit.accessibleDescription().isEmpty());
    }

    void accessibleDescriptionUpdatesOnSequenceChange()
    {
        TKeySequenceEdit edit(QKeySequence(), qsl("Open file"));
        auto* lineEdit = innerLineEdit(edit);
        const QString unsetDescription = lineEdit->accessibleDescription();
        QVERIFY(!unsetDescription.isEmpty());

        // Once there is a value to announce, the description is cleared so the
        // binding is not read out twice:
        edit.setKeySequence(QKeySequence(qsl("Alt+F4")));
        QVERIFY(lineEdit->accessibleDescription().isEmpty());

        edit.clear();
        QCOMPARE(lineEdit->accessibleDescription(), unsetDescription);
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
    // widget commits in focusOutEvent() when Tab moves focus away. Qt only
    // delivers those focus events while the window is active, and nothing
    // activates a window on an X server without a window manager, so on such a
    // display these two cases are skipped rather than failed (#9575). Under
    // ctest they never get that far: the offscreen platform is pinned there,
    // and it synthesises activation.
    void shiftBacktabCommitsCaptureAndMovesFocusBackwards()
    {
        QWidget window;
        auto* layout = new QVBoxLayout(&window);
        auto* neighbour = new QLineEdit(&window);
        auto* edit = new TKeySequenceEdit(QKeySequence(), qsl("Open file"), &window);
        layout->addWidget(neighbour);
        layout->addWidget(edit);
        window.show();
        REQUIRE_WINDOW_ACTIVATION(window);

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
        REQUIRE_WINDOW_ACTIVATION(window);

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
