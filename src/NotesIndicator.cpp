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


#include "NotesIndicator.h"
#include "NotesManager.h"

#include <QApplication>
#include <QMouseEvent>
#include <QStyle>

NotesIndicator::NotesIndicator(QWidget* pParent)
: QPushButton(pParent)
{
    setObjectName(qsl("notesIndicator"));

    QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    policy.setHorizontalStretch(0);
    policy.setVerticalStretch(0);
    policy.setHeightForWidth(false);
    setSizePolicy(policy);

    setSize(mIconSize);
    loadIcons();

    connect(this, &QPushButton::clicked, this, [this]() {
        if (mState != State::Empty) {
            emit notesButtonClicked();
        }
    });
}

NotesIndicator::~NotesIndicator()
{
    disconnectFromNotesManager();
}

void NotesIndicator::setNotesManager(NotesManager* pManager)
{
    if (mpNotesManager == pManager) {
        return;
    }

    disconnectFromNotesManager();
    mpNotesManager = pManager;
    connectToNotesManager();
    updateState();
    updateNoteCount();
}

void NotesIndicator::setCurrentTabId(const QString& tabId)
{
    if (mCurrentTabId != tabId) {
        mCurrentTabId = tabId;
        updateState();
    }
}

void NotesIndicator::setTabVisible(bool visible)
{
    if (mIsTabVisible != visible) {
        mIsTabVisible = visible;
        if (visible) {
            resetUnreadState();
        }
        updateState();
    }
}

void NotesIndicator::resetUnreadState()
{
    if (mState == State::HasUnread) {
        setState(State::HasContent);
    }
}

void NotesIndicator::updateState()
{
    if (!mpNotesManager) {
        setState(State::Empty);
        return;
    }

    const auto& tabsMap = mpNotesManager->getTabsMap();
    const int tabCount = tabsMap.size();

    if (tabCount == 0) {
        setState(State::Empty);
        return;
    }

    bool hasDirtyTabs = false;
    for (const auto& tab : tabsMap) {
        if (tab.isDirty) {
            hasDirtyTabs = true;
            break;
        }
    }

    if (hasDirtyTabs) {
        if (mIsTabVisible) {
            setState(State::Modified);
        } else {
            setState(State::HasUnread);
        }
    } else {
        setState(State::HasContent);
    }
}

void NotesIndicator::updateNoteCount()
{
    if (!mpNotesManager) {
        setNoteCount(0);
        return;
    }

    const int count = mpNotesManager->getTabsMap().size();
    setNoteCount(count);
}

void NotesIndicator::connectToNotesManager()
{
    if (!mpNotesManager) {
        return;
    }

    connect(mpNotesManager, &NotesManager::tabAdded, this, &NotesIndicator::slotTabAdded);
    connect(mpNotesManager, &NotesManager::tabRemoved, this, &NotesIndicator::slotTabRemoved);
    connect(mpNotesManager, &NotesManager::tabRenamed, this, &NotesIndicator::slotTabRenamed);
    connect(mpNotesManager, &NotesManager::contentChanged, this, &NotesIndicator::slotContentChanged);
}

void NotesIndicator::disconnectFromNotesManager()
{
    if (mpNotesManager) {
        disconnect(mpNotesManager, nullptr, this, nullptr);
    }
}

void NotesIndicator::slotTabAdded(const QString& tabId, const QString& tabName)
{
    Q_UNUSED(tabId);
    Q_UNUSED(tabName);

    updateState();
    updateNoteCount();
}

void NotesIndicator::slotTabRemoved(const QString& tabId)
{
    Q_UNUSED(tabId);

    if (mCurrentTabId == tabId) {
        mCurrentTabId.clear();
    }

    updateState();
    updateNoteCount();
}

void NotesIndicator::slotTabRenamed(const QString& tabId, const QString& newName)
{
    Q_UNUSED(tabId);
    Q_UNUSED(newName);
    updateState();
}

void NotesIndicator::slotContentChanged(const QString& tabId)
{
    Q_UNUSED(tabId);

    updateState();
}

void NotesIndicator::setState(State state)
{
    if (mState != state) {
        mState = state;
        updateIcon();
    }
}

void NotesIndicator::setSize(int size)
{
    if (mIconSize != size && size > 0) {
        mIconSize = size;
        setFixedSize(size, size);
        loadIcons();
    }
}

void NotesIndicator::setNoteCount(int count)
{
    if (mNoteCount != count) {
        mNoteCount = count;
        updateIcon();
    }
}

void NotesIndicator::loadIcons()
{
    mIcons.clear();

    QList<State> states = {State::Empty, State::HasContent, State::Modified, State::HasUnread};

    for (const State state : states) {
        QString resourcePath;

        switch (state) {
        case State::Empty:
            resourcePath = qsl(":/icons/note-empty.png");
            break;
        case State::HasContent:
            resourcePath = qsl(":/icons/note-present.png");
            break;
        case State::Modified:
            resourcePath = qsl(":/icons/note-active.png");
            break;
        case State::HasUnread:
            resourcePath = qsl(":/icons/note-hover.png");
            break;
        }

        QIcon icon;
        QPixmap pixmap(resourcePath);

        if (!pixmap.isNull()) {
            icon = QIcon(pixmap.scaled(mIconSize, mIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            switch (state) {
            case State::Empty:
                icon = style()->standardIcon(QStyle::SP_FileIcon);
                break;
            case State::HasContent:
                icon = style()->standardIcon(QStyle::SP_FileDialogContentsView);
                break;
            case State::Modified:
                icon = style()->standardIcon(QStyle::SP_DialogSaveButton);
                break;
            case State::HasUnread:
                icon = style()->standardIcon(QStyle::SP_DialogHelpButton);
                break;
            }
        }

        mIcons.insert(state, icon);
    }

    updateIcon();
}

void NotesIndicator::updateIcon()
{
    if (mIcons.contains(mState)) {
        setIcon(mIcons.value(mState));
        setIconSize(QSize(mIconSize, mIconSize));
    }
}

void NotesIndicator::mousePressEvent(QMouseEvent* pEvent)
{
    if (mState == State::Empty) {
        return;
    }
    QPushButton::mousePressEvent(pEvent);
}
