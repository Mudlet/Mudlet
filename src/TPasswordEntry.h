#ifndef MUDLET_TPASSWORDENTRY_H
#define MUDLET_TPASSWORDENTRY_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include <QLineEdit>
#include <QPointer>

class Host;
class QAction;
class TCommandLine;

// Answers the game's request for hidden input (IAC WILL ECHO) over the main
// command line, so what is typed never enters the command line, its history,
// completion, aliases or anything a script can read.
//
// Only submit() may read the text, beyond asking whether it is empty. QLineEdit's
// public text() cannot enforce that, so TMainConsole hands the widget to nothing
// but its tests, and deletes it at the end of each prompt so Qt zero-fills what
// it still holds.
class TPasswordEntry : public QLineEdit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TPasswordEntry)
    explicit TPasswordEntry(Host* pHost, TCommandLine* pCommandLine, QWidget* parent);

    // For a box that follows an Esc in the same ECHO hold
    void setReopened();

signals:
    void submitted();
    void dismissed();

public slots:
    void slot_adjustAccessibleNames();

private slots:
    void slot_selectionClipboardChanged();

private:
    bool event(QEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void handleKeyPress(QKeyEvent*);
    void submit();
    void setRevealed(const bool revealed);
    void scrollConsole(const bool up);
    bool copyConsoleSelection();

    QPointer<Host> mpHost;
    QPointer<TCommandLine> mpCommandLine;
    QAction* mpRevealAction = nullptr;
    bool mReopened = false;
};

#endif // MUDLET_TPASSWORDENTRY_H
