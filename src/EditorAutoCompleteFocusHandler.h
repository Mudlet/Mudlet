#ifndef MUDLET_EDITORAUTOCOMPLETEFOCUSHANDLER_H
#define MUDLET_EDITORAUTOCOMPLETEFOCUSHANDLER_H

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

#include <QObject>

class QKeyEvent;
class QListWidget;
class QWidget;

namespace edbee {
class TextEditorWidget;
}

/*
 * Keeps the keyboard focus in an edbee editor while its autocomplete popup is
 * open, and routes the keys the popup receives to where they belong.
 *
 * edbee's TextEditorAutoCompleteComponent::updateList() calls setFocus() on
 * the suggestion list every time the popup is shown or refreshed, which moves
 * the keyboard focus (and, on some platforms, the window activation) out of
 * the editor. Typing then only works through edbee's event-filter forwarding,
 * which loses keys and can leave the editor unusable until the popup is
 * dismissed (see Mudlet issue #5310).
 *
 * The approach is QCompleter's: the popup and its list are made non-focusable
 * and non-activating so the editor keeps the focus at all times, and an event
 * filter on the popup routes each key to the list (completion keys) or to the
 * editor (everything else, normal typing included).
 *
 * Every editor that shows a completion list needs this - the script editor's
 * own widget and the preview in the preferences dialog - so it lives here
 * rather than in either of them.
 */
class EditorAutoCompleteFocusHandler : public QObject
{
    Q_OBJECT

public:
    explicit EditorAutoCompleteFocusHandler(edbee::TextEditorWidget* pEditor, QObject* pParent = nullptr);

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    // Hands a key the list cannot act on back, so routing it again would loop.
    // edbee parents the list to the popup menu, and QApplication::notify()
    // passes an ignored key to the next widget up the parent chain, so an
    // end-of-list arrow key arrives here twice: once from Qt, once from us.
    bool isBeingRouted() const { return mRoutingKey; }

    void routeKeyPress(QKeyEvent* pKeyEvent);
    bool isPopupOpen() const;

    QListWidget* mpList = nullptr;
    QWidget* mpMenu = nullptr;
    QWidget* mpEditorComponent = nullptr;
    bool mRoutingKey = false;
};

#endif // MUDLET_EDITORAUTOCOMPLETEFOCUSHANDLER_H
