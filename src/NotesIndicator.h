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


#include <QColor>
#include <QIcon>
#include <QMap>
#include <QPointer>
#include <QPushButton>
#include <QString>
#include <QTimer>

class NotesManager;

class QContextMenuEvent;
class QEnterEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPropertyAnimation;
class QTimer;

class QMenu;
class QAction;

class NotesIndicator : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(int displayIconSize READ displayIconSize WRITE setDisplayIconSize)
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

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
    int size() const { return mBaseIconSize; }

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
    void newNoteRequested();
    void viewNotesRequested();

protected:
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void enterEvent(QEnterEvent* pEvent) override;
    void leaveEvent(QEvent* pEvent) override;
    void changeEvent(QEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;
    void contextMenuEvent(QContextMenuEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;

private:
    void loadIcons();
    void updateIcon();
    void updateToolTip();

    void invalidateTooltipCache();
    void scheduleTooltipUpdate();
    void onTooltipUpdateTimeout();
    QString generateRichTooltip() const;

    bool isClickable() const;

    void setupContextMenu();
    void updateContextMenuState();
    QMenu* createContextMenu();

    QAction* mActionNewNote = nullptr;
    QAction* mViewNotesAction = nullptr;
    QAction* mActionRenameTab = nullptr;
    QAction* mActionDeleteTab = nullptr;
    QAction* mActionClearTab = nullptr;

private slots:
    void slotNewNote();
    void slotViewNotes();
    void slotRenameTab();
    void slotDeleteTab();
    void slotClearTab();

    void animateIconSizeTo(int targetSize);
    void animateHoverProgressTo(qreal targetProgress);

    int displayIconSize() const { return mDisplayIconSize; }
    void setDisplayIconSize(int size);

    qreal hoverProgress() const { return mHoverProgress; }
    void setHoverProgress(qreal progress);

    QColor badgeColorForState(State state) const;
    QIcon makeIconWithBadge(const QIcon& baseIcon, const QColor& badgeColor) const;

    bool isDarkTheme() const;
    bool isHighContrastTheme() const;

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
private:
    State mState = State::Empty;
    int mBaseIconSize = 16;
    int mNoteCount = 0;

    int mDisplayIconSize = 16;
    qreal mHoverProgress = 0.0;

    bool mIsHovered = false;
    bool mIsPressed = false;

    QMap<State, QIcon> mIcons;

    QPropertyAnimation* mpIconSizeAnimation = nullptr;
    QPropertyAnimation* mpHoverAnimation = nullptr;

    // NotesManager integration
    QPointer<NotesManager> mpNotesManager;
    QString mCurrentTabId;
    bool mIsTabVisible = false;

    // Rich tooltip caching and throttling
    QString mCachedTooltip;
    bool mTooltipCacheValid = false;
    QTimer mTooltipUpdateTimer;
    bool mTooltipUpdatePending = false;
};

#endif // MUDLET_NOTESINDICATOR_H
