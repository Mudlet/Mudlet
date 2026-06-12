#ifndef TKEYSEQUENCEEDIT_H
#define TKEYSEQUENCEEDIT_H

/***************************************************************************
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

#include <QKeySequenceEdit>

class QLineEdit;

// A QKeySequenceEdit that works with screen readers and keyboard navigation:
// - keyboard focus is routed to the inner QLineEdit (a real text control that
//   assistive technologies announce with both its name and the current
//   binding), with key events forwarded back into the capture logic
// - lone modifier presses do not start a capture, so traversing the control
//   with Shift+Tab no longer erases the stored binding (the stock widget
//   clears itself on the first key press of a capture - even a bare Shift)
// - Shift+Tab/Backtab combinations finish the capture and move focus instead
//   of being recorded as a binding
class TKeySequenceEdit : public QKeySequenceEdit
{
    Q_OBJECT

public:
    explicit TKeySequenceEdit(const QKeySequence& sequence, const QString& accessibleLabel, QWidget* pParent = nullptr);

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;

private slots:
    void slot_updateAccessibleDescription();

private:
    QLineEdit* mpLineEdit = nullptr;
};

#endif // TKEYSEQUENCEEDIT_H
