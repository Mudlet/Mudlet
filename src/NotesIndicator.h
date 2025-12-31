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
#include <QPointer>
#include <QPushButton>
#include <QString>

class NotesManager;

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
    ~NotesIndicator();

    Q_DISABLE_COPY(NotesIndicator)

    void setState(State state);
    State getState() const { return mState; }

    void setSize(int size);
    int size() const { return mIconSize; }

    void setNoteCount(int count);
    int noteCount() const { return mNoteCount; }

    // NotesManager integration
    void setNotesManager(NotesManager* pManager);
    NotesManager* notesManager() const { return mpNotesManager.data(); }

    void setCurrentTabId(const QString& tabId);
    QString currentTabId() const { return mCurrentTabId; }

    void setTabVisible(bool visible);
    bool isTabVisible() const { return mIsTabVisible; }

    void resetUnreadState();

signals:
    void notesButtonClicked();

protected:
    void mousePressEvent(QMouseEvent* pEvent) override;

private:
    void loadIcons();
    void updateIcon();

    // State management
    void updateState();
    void updateNoteCount();

    // NotesManager signal handlers
    void slotTabAdded(const QString& tabId, const QString& tabName);
    void slotTabRemoved(const QString& tabId);
    void slotTabRenamed(const QString& tabId, const QString& newName);
    void slotContentChanged(const QString& tabId);

    // NotesManager connection management
    void connectToNotesManager();
    void disconnectFromNotesManager();

    State mState = State::Empty;
    int mIconSize = 16;
    int mNoteCount = 0;
    QMap<State, QIcon> mIcons;

    // NotesManager integration
    QPointer<NotesManager> mpNotesManager;
    QString mCurrentTabId;
    bool mIsTabVisible = false;
};

#endif // MUDLET_NOTESINDICATOR_H
