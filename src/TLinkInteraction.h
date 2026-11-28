#ifndef MUDLET_TLINKINTERACTION_H
#define MUDLET_TLINKINTERACTION_H

/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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
#include <QString>

class QFont;
class QMenu;
class QWidget;
class TBuffer;
class TConsoleModel;

// What a view does with a link under the pointer, the caret or a menu, in terms of the buffer and the model;
// the view draws, positions the menu and places the tooltip
class TLinkInteraction : public QObject
{
    Q_OBJECT

public:
    TLinkInteraction(TBuffer& buffer, TConsoleModel& model, QObject* parent);

    // text: treat it as a press on text; redraw: repaint only; ran: ran, repaint; menu: ran, repaint and show its menu
    enum class Press { text, redraw, ran, menu };
    Press press(int linkIndex);
    // text: the menu any other text gets; revealed: repaint; blocked: no menu at all; menu: show the link's menu
    enum class RightClick { text, revealed, blocked, menu };
    RightClick rightClick(int linkIndex);
    // none: not a link that takes keys; ignored: the key is the view's to handle as any other; redraw: repaint only;
    // ran: nothing more to do; menu: show the link's menu at the caret
    enum class Activate { none, ignored, redraw, ran, menu };
    // Enter or Space on the focused link
    Activate activateFocused();
    // The Menu key on the focused link; none means the console's own menu, ignored means nothing
    Activate menuKeyOnFocused();
    // Whether the hover state changed and the view must repaint; 0 for no link under the pointer
    bool hoverAt(int linkIndex);
    // The pointer left the view: whether a hovered link lost its hover style
    bool leave();
    Qt::CursorShape cursorFor(int linkIndex) const;
    // The tooltip as markup QToolTip will not reinterpret, empty for none
    QString tooltipMarkup(int linkIndex) const;
    // The link's menu, in the view's font; picks from it run on the link it was built for
    QMenu* buildMenu(int linkIndex, QWidget* parent, const QFont& font, bool alreadyActivated);
    QString announcement(int linkIndex) const;
    void announceFocus(int linkIndex);

signals:
    // Around a menu pick, as the item may open a dialog whose keys the view's keyboard grab would keep
    void aboutToRunMenuPick();
    void menuPickRan();

private:
    void runPick(int item);

    TBuffer& mBuffer;
    TConsoleModel& mModel;
    // The link whose menu is showing, 0 for none; its actions carry the item's index plus one
    int mPopupLinkIndex = 0;
    // Its TLinkStore::issueOf() as the menu was built, as a pick must not run a link that has since taken its id
    qint64 mPopupLinkIssue = 0;
    // Activating a local: link with a menu opens the menu, so choosing an item must not change its selection a second time
    bool mPopupLinkAlreadyActivated = false;
};

#endif // MUDLET_TLINKINTERACTION_H
