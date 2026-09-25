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

// The box the game's request for hidden input (IAC WILL ECHO) is answered
// with. It sits over the main command line and takes its keyboard while the
// game holds ECHO, so that what is typed there never enters the command line:
// no history, no completion, no aliases, no sysDataSendRequest, nothing for a
// script to read. Enter hands its text to Host::sendPasswordEntry(), the one
// path to the wire; Esc empties it, and Esc on an empty box steps past it.
//
// The text is read in exactly one place, submit(); there is no accessor and no
// signal carries it. TMainConsole creates one when Host::passwordEntryWanted()
// turns true and deletes it when that turns false, so nothing carries over from
// one prompt to the next, and Qt zero-fills the text a password-mode line edit
// still holds when it is destroyed.
class TPasswordEntry : public QLineEdit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TPasswordEntry)
    explicit TPasswordEntry(Host* pHost, TCommandLine* pCommandLine, QWidget* parent);

    // Switches the wording to that of a box which follows an Esc in the same
    // ECHO hold, so the player learns that a second Esc lasts until the game
    // releases ECHO.
    void setReopened();

signals:
    // Enter was pressed and the text handed to the send path. No text rides on it.
    void submitted();
    // Esc was pressed on an empty box.
    void dismissed();

public slots:
    void slot_adjustAccessibleNames();

private:
    bool event(QEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void handleKeyPress(QKeyEvent*);
    void submit();
    void setRevealed(const bool revealed);
    void applyInputMethodHints();
    void scrollConsole(const bool up);

    QPointer<Host> mpHost;
    QPointer<TCommandLine> mpCommandLine;
    QAction* mpRevealAction = nullptr;
    bool mReopened = false;
};

#endif // MUDLET_TPASSWORDENTRY_H
