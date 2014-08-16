/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef SESSIONTREEWIDGET_H
#define SESSIONTREEWIDGET_H

#include <QTreeWidget>
#include <QShortcut>
#include <QColor>
#include <QHash>
#include "settings.h"

class Session;
struct Settings;
class SessionTreeItem;

class SessionTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    SessionTreeWidget(QWidget* parent = 0);

    QSize sizeHint() const;

    enum ItemStatus { Active, Inactive, Alert, Highlight };

    QColor statusColor(ItemStatus status) const;
    void setStatusColor(ItemStatus status, const QColor& color);

    QColor currentAlertColor() const;

    QList<Session*> sessions() const;
    SessionTreeItem* sessionItem(Session* session) const;

public slots:
    void addSession(Session* session);
    void removeSession(Session* session);

    void addView(Session* session, const QString& view);
    void removeView(Session* session, const QString& view);
    void renameView(Session* session, const QString& from, const QString& to);
    void setCurrentView(Session* session, const QString& view = QString());

    void moveToNextItem();
    void moveToPrevItem();

    void moveToNextUnreadItem();
    void moveToPrevUnreadItem();

    void expandCurrentSession();
    void collapseCurrentSession();

    void alert(SessionTreeItem* item);
    void unalert(SessionTreeItem* item);

    void applySettings(const Settings& settings);

signals:
    void currentViewChanged(Session* session, const QString& view);
    void menuRequested(SessionTreeItem* item, const QPoint& pos);

protected:
    void contextMenuEvent(QContextMenuEvent* event);
    bool event(QEvent* event);

private slots:
    void updateSession(Session* session = 0);
    void onItemSelectionChanged();
    void onItemExpanded(QTreeWidgetItem* item);
    void onItemCollapsed(QTreeWidgetItem* item);
    void delayedItemReset();
    void delayedItemResetTimeout();
    void alertTimeout();

private:
    void resetItem(SessionTreeItem* item);
    QTreeWidgetItem* lastItem() const;
    QTreeWidgetItem* nextItem(QTreeWidgetItem* from) const;
    QTreeWidgetItem* previousItem(QTreeWidgetItem* from) const;

    struct SessionTreeWidgetData
    {
        QColor alertColor;
        Settings settings;
        QShortcut* prevShortcut;
        QShortcut* nextShortcut;
        QShortcut* prevUnreadShortcut;
        QShortcut* nextUnreadShortcut;
        QShortcut* expandShortcut;
        QShortcut* collapseShortcut;
        QHash<ItemStatus, QColor> colors;
        QSet<SessionTreeItem*> alertedItems;
        QSet<SessionTreeItem*> resetedItems;
        QHash<Session*, SessionTreeItem*> sessions;
    } d;
    friend class SessionTreeItem;
};

#endif // SESSIONTREEWIDGET_H
