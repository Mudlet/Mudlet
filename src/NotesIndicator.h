#ifndef MUDLET_NOTESINDICATOR_H
#define MUDLET_NOTESINDICATOR_H

/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Development Team                         *
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


#include <QIcon>
#include <QMap>
#include <QPushButton>

class NotesIndicator : public QPushButton
{
    Q_OBJECT

public:
    enum State {
        Empty,
        HasContent,
        Modified,
        HasUnread
    };

    Q_ENUM(State)

    explicit NotesIndicator(QWidget* pParent = nullptr);
    ~NotesIndicator() = default;

    Q_DISABLE_COPY(NotesIndicator)

    void setState(State state);
    State getState() const { return mState; }

    void setSize(int size);
    int size() const { return mIconSize; }

    void setNoteCount(int count);
    int noteCount() const { return mNoteCount; }

signals:
    void notesButtonClicked();

protected:
    void mousePressEvent(QMouseEvent* pEvent) override;

private:
    void loadIcons();
    void updateIcon();

    State mState = State::Empty;
    int mIconSize = 16;
    int mNoteCount = 0;
    QMap<State, QIcon> mIcons;
};

#endif // MUDLET_NOTESINDICATOR_H
