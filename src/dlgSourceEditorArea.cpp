/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
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


#include "dlgSourceEditorArea.h"

#include "utils.h"

#include "edbee/edbee.h"
#include "edbee/models/textdocument.h"
#include "edbee/texteditorcomponent.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/models/textgrammar.h"
#include "edbee/views/components/texteditorautocompletecomponent.h"
#include "edbee/views/texteditorscrollarea.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/texttheme.h"

#include <QApplication>
#include <QKeyEvent>
#include <QListWidget>

dlgSourceEditorArea::dlgSourceEditorArea(QWidget* pParentWidget)
: QWidget(pParentWidget)
{
    // init generated dialog
    setupUi(this);

    // Configuring the editor widget with defaults

    edbee::TextEditorConfig* config = edbeeEditorWidget->config();

    config->beginChanges();

    config->setSmartTab(true);    // enable the automatic addition of indents when inserting a newline
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);

    config->setIndentSize(2); // 2 spaces is the Lua default
    config->setCaretWidth(1);

    config->endChanges();

    edbeeEditorWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(qsl("Buck.lua")));

    // disable shadows as their purpose (notify there is more text) is performed by scrollbars already
    edbeeEditorWidget->textScrollArea()->enableShadowWidget(false);

    // keep the keyboard focus with the editor while the autocomplete popup is shown
    configureAutoCompleteFocus();
}

// The autocomplete popup used to steal the keyboard focus from the editor:
// edbee's TextEditorAutoCompleteComponent::updateList() calls setFocus() on its
// list widget every time the popup is shown or refreshed, which moves the
// keyboard focus (and, on some platforms, the window activation) out of the
// editor while the popup is open. Typing then only works through edbee's
// event-filter forwarding, which loses keys and can leave the editor unusable
// until the popup is dismissed (see Mudlet issue #5310).
//
// Use the same approach as QCompleter instead: the popup and its list are made
// non-focusable and non-activating so the editor keeps the focus at all times.
// While the popup is open Qt delivers the key events to the popup widget, so
// eventFilter() routes the completion keys to the popup list and everything
// else (normal typing included) to the editor.
void dlgSourceEditorArea::configureAutoCompleteFocus()
{
    auto* autoCompleteComponent = edbeeEditorWidget->autoCompleteComponent();
    if (!autoCompleteComponent) {
        return;
    }

    mpAutoCompleteList = autoCompleteComponent->listWidget();
    if (!mpAutoCompleteList) {
        return;
    }

    mpAutoCompleteMenu = mpAutoCompleteList->parentWidget();
    mpEditorComponent = edbeeEditorWidget->textEditorComponent();
    if (!mpAutoCompleteMenu || !mpEditorComponent) {
        return;
    }

    // Making the list's focus proxy the editor component turns edbee's
    // setFocus() call on the list into a no-op: QWidget::setFocus() resolves it
    // to the focus proxy, which already owns the keyboard focus, so nothing
    // changes. The widgets are additionally marked as non-activating so the
    // popup window cannot take the window activation (notably on Windows).
    mpAutoCompleteList->setFocusProxy(mpEditorComponent);
    mpAutoCompleteList->setFocusPolicy(Qt::NoFocus);
    mpAutoCompleteList->setAttribute(Qt::WA_ShowWithoutActivating);

    mpAutoCompleteMenu->setFocusPolicy(Qt::NoFocus);
    mpAutoCompleteMenu->setAttribute(Qt::WA_ShowWithoutActivating);

    mpAutoCompleteMenu->installEventFilter(this);
    mpEditorComponent->installEventFilter(this);
}

// Routes a key press that was received by the autocomplete popup. Every key is
// consumed here: it is either handed to the popup list (completion keys) or to
// the editor (normal typing), never back to the popup menu itself, whose own
// key handling would close the popup or eat the key.
void dlgSourceEditorArea::routeAutoCompleteKeyPress(QKeyEvent* pKeyEvent)
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
        QApplication::sendEvent(mpAutoCompleteList, pKeyEvent);
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
    mpAutoCompleteMenu->close();
    QApplication::sendEvent(mpEditorComponent, pKeyEvent);
}

bool dlgSourceEditorArea::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (mpAutoCompleteMenu && pWatched == mpAutoCompleteMenu) {
        switch (pEvent->type()) {
        case QEvent::KeyPress:
            if (mpAutoCompleteMenu->isVisible()) {
                routeAutoCompleteKeyPress(static_cast<QKeyEvent*>(pEvent));
                return true;
            }
            break;

        case QEvent::InputMethod:
        case QEvent::ShortcutOverride:
            // input method and shortcut overrides are handled by the editor,
            // not by the popup
            if (mpAutoCompleteMenu->isVisible()) {
                QApplication::sendEvent(mpEditorComponent, pEvent);
                return true;
            }
            break;

        default:
            break;
        }

    } else if (pWatched == mpEditorComponent && pEvent->type() == QEvent::FocusOut
               && mpAutoCompleteMenu && mpAutoCompleteMenu->isVisible()) {
        // Qt sends a synthetic FocusOut to the current focus widget when the
        // popup opens; swallow it so that the editor does not reset its undo
        // coalescing (or drop the IME state) every time the popup is shown
        // during typing
        return true;
    }

    return QWidget::eventFilter(pWatched, pEvent);
}
