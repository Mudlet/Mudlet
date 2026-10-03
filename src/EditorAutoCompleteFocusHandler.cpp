/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                                  *
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

#include "EditorAutoCompleteFocusHandler.h"

#include "edbee/texteditorwidget.h"
#include "edbee/views/components/texteditorautocompletecomponent.h"
#include "edbee/views/components/texteditorcomponent.h"

#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QScopedValueRollback>

EditorAutoCompleteFocusHandler::EditorAutoCompleteFocusHandler(edbee::TextEditorWidget* pEditor, QObject* pParent)
: QObject(pParent)
{
    if (!pEditor) {
        return;
    }

    auto* autoCompleteComponent = pEditor->autoCompleteComponent();
    if (!autoCompleteComponent) {
        return;
    }

    mpList = autoCompleteComponent->listWidget();
    if (!mpList) {
        return;
    }

    mpMenu = mpList->parentWidget();
    mpEditorComponent = pEditor->textEditorComponent();
    if (!mpMenu || !mpEditorComponent) {
        return;
    }

    // Making the list's focus proxy the editor component turns edbee's
    // setFocus() call on the list into a no-op: QWidget::setFocus() resolves it
    // to the focus proxy, which already owns the keyboard focus, so nothing
    // changes. The list is also marked non-focusable, so nothing else can hand
    // the keyboard to it.
    mpList->setFocusProxy(mpEditorComponent);
    mpList->setFocusPolicy(Qt::NoFocus);

    // The attribute is only consulted for a top-level window - QWidget only
    // checks it in show() when isWindow() is true - so it belongs on the menu
    // alone. Setting it on the list, which edbee parents to the menu, has never
    // had any effect.
    mpMenu->setFocusPolicy(Qt::NoFocus);
    mpMenu->setAttribute(Qt::WA_ShowWithoutActivating);

    mpMenu->installEventFilter(this);
    mpEditorComponent->installEventFilter(this);
}

bool EditorAutoCompleteFocusHandler::isPopupOpen() const
{
    return mpMenu && mpMenu->isVisible();
}

// Routes a key press that was received by the autocomplete popup. Every key is
// consumed here: it is either handed to the popup list (completion keys) or to
// the editor (normal typing), never back to the popup menu itself, whose own
// key handling would close the popup or eat the key.
void EditorAutoCompleteFocusHandler::routeKeyPress(QKeyEvent* pKeyEvent)
{
    switch (pKeyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Tab:
    case Qt::Key_Escape:
        // completion keys, handled by edbee's event filter on the list
        // (navigate the suggestions, accept or cancel the popup)
        QApplication::sendEvent(mpList, pKeyEvent);
        return;

    case Qt::Key_Backspace:
    case Qt::Key_Shift:
        // keep the popup open while deleting text or holding Shift
        QApplication::sendEvent(mpEditorComponent, pKeyEvent);
        return;

    default:
        break;
    }

    if (!pKeyEvent->text().isEmpty() && pKeyEvent->text().at(0).isLetterOrNumber()) {
        // normal typing: the editor handles the key and refreshes the popup
        QApplication::sendEvent(mpEditorComponent, pKeyEvent);
        return;
    }

    // any other key closes the popup and is then handled by the editor
    mpMenu->close();
    QApplication::sendEvent(mpEditorComponent, pKeyEvent);
}

bool EditorAutoCompleteFocusHandler::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (mpMenu && pWatched == mpMenu) {
        switch (pEvent->type()) {
        case QEvent::KeyPress:
            if (isPopupOpen()) {
                // A navigation key the list cannot act on - Up on the first
                // suggestion, Down on the last, any arrow on a single-item list
                // - is left unhandled by QAbstractItemView, and
                // QApplication::notify() then hands the ignored event to the
                // next widget up the parent chain, which is this menu. So this
                // filter sees the same key a second time, already routed once.
                // Dropping it is what stops the routing from repeating until
                // the stack runs out.
                if (isBeingRouted()) {
                    return true;
                }
                QScopedValueRollback<bool> guard(mRoutingKey, true);
                routeKeyPress(static_cast<QKeyEvent*>(pEvent));
                return true;
            }
            break;

        case QEvent::InputMethod:
        case QEvent::ShortcutOverride:
            // input method and shortcut overrides are handled by the editor,
            // not by the popup
            if (isPopupOpen()) {
                QApplication::sendEvent(mpEditorComponent, pEvent);
                return true;
            }
            break;

        default:
            break;
        }

    } else if (pWatched == mpEditorComponent && pEvent->type() == QEvent::FocusOut && isPopupOpen()) {
        // Opening a popup makes Qt send a synthetic FocusOut with
        // Qt::PopupFocusReason to whatever has the keyboard focus, and edbee's
        // TextEditorComponent::focusOutEvent() answers it by resetting the
        // undo coalescing ids - which would split a word being typed into
        // several undo steps, one per popup refresh. Swallow just that one:
        // every other reason is a real focus change, and swallowing those
        // would leave the editor thinking it still has the focus.
        if (static_cast<QFocusEvent*>(pEvent)->reason() == Qt::PopupFocusReason) {
            return true;
        }
    }

    return QObject::eventFilter(pWatched, pEvent);
}
