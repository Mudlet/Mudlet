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

#include <QKeyEvent>
#include <QLineEdit>

TKeySequenceEdit::TKeySequenceEdit(const QKeySequence& sequence, const QString& accessibleLabel, QWidget* pParent)
: QKeySequenceEdit(sequence, pParent)
, mpLineEdit(findChild<QLineEdit*>())
{
    // Only the inner QLineEdit - the control that actually receives focus - is
    // given an accessible name (below); the wrapper is deliberately left
    // unnamed so a screen reader announces the label once, for the focused
    // field, rather than a second time for the wrapper's grouping (#9322). The
    // preferences page points the buddy QLabel at that same inner control for
    // the same reason.
    //
    // The inner QLineEdit is a Qt implementation detail; if it ever goes away
    // we degrade to the stock (less accessible) behaviour instead of crashing:
    if (mpLineEdit) {
        // Route keyboard focus to the inner QLineEdit instead of this
        // container: assistive technologies announce a focused text control
        // together with its content, whereas the stock arrangement focuses
        // the container which exposes neither the label nor the current
        // binding. The key events are forwarded back to this class by the
        // event filter below, so the capture behaviour is unchanged:
        mpLineEdit->setFocusProxy(nullptr);
        setFocusProxy(mpLineEdit);
        mpLineEdit->installEventFilter(this);
        // The binding must not be editable by any route other than the
        // capture logic - read-only also blocks drag&drop and middle-click
        // paste, while the event filter intercepts keys before the read-only
        // check so capturing still works:
        mpLineEdit->setReadOnly(true);
        mpLineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
        mpLineEdit->setContextMenuPolicy(Qt::NoContextMenu);
        mpLineEdit->setAccessibleName(accessibleLabel);
    } else {
        // Degraded fallback: with no inner field to carry the name, label the
        // wrapper itself so the control is not left unnamed:
        setAccessibleName(accessibleLabel);
    }

    slot_updateAccessibleDescription();
    connect(this, &QKeySequenceEdit::keySequenceChanged, this, &TKeySequenceEdit::slot_updateAccessibleDescription);

    // By default only an unmodified Tab or Backtab ends the key capture, but
    // Shift+Tab arrives as Backtab plus the Shift modifier so it would get
    // recorded as the new binding instead of moving focus backwards (#8873):
    auto combinations = finishingKeyCombinations();
    combinations << QKeyCombination(Qt::ShiftModifier, Qt::Key_Backtab) << QKeyCombination(Qt::ShiftModifier, Qt::Key_Tab);
    setFinishingKeyCombinations(combinations);
}

bool TKeySequenceEdit::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (pWatched != mpLineEdit) {
        return QKeySequenceEdit::eventFilter(pWatched, pEvent);
    }

    switch (pEvent->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        // Run the capture logic; for the finishing combinations event() falls
        // through to QWidget::event() which performs the Tab/Backtab focus
        // navigation:
        event(pEvent);
        return true;
    case QEvent::FocusOut:
        // The base class commits a pending capture when it loses focus, but
        // with the focus living on the inner QLineEdit its focusOutEvent()
        // never runs - replicate it, then let the line edit process the event
        // as well:
        focusOutEvent(static_cast<QFocusEvent*>(pEvent));
        return false;
    default:
        return QKeySequenceEdit::eventFilter(pWatched, pEvent);
    }
}

void TKeySequenceEdit::keyPressEvent(QKeyEvent* pEvent)
{
    const int key = pEvent->key();
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta) {
        // The base class treats any key press as the start of a capture and
        // clears the current binding, so a bare modifier press - the first
        // half of a Shift+Tab focus traversal - would erase the shortcut.
        // Modifiers only contribute to a binding via the modifier flags on
        // the non-modifier key press, so they are safe to ignore here:
        pEvent->accept();
        return;
    }
    QKeySequenceEdit::keyPressEvent(pEvent);
}

void TKeySequenceEdit::slot_updateAccessibleDescription()
{
    // The inner QLineEdit shows the binding as its text, which a screen reader
    // announces as the field's value. Repeating that binding in the accessible
    // description - and a second time on this wrapper widget - made readers
    // speak the combination several times per editor (#9322). Only describe the
    // empty state, where there is no value for the reader to fall back on:
    QString description;
    if (keySequence().isEmpty()) {
        //: Accessibility description of a keyboard shortcut editor in the preferences when no shortcut is assigned
        description = tr("No shortcut set");
    }
    if (mpLineEdit) {
        mpLineEdit->setAccessibleDescription(description);
    }
}
