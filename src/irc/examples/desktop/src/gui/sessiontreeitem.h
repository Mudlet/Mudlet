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

#ifndef SESSIONTREEITEM_H
#define SESSIONTREEITEM_H

#include <QTreeWidgetItem>

class Session;

class SessionTreeItem : public QTreeWidgetItem
{
public:
    SessionTreeItem(Session* session, QTreeWidget* parent);
    SessionTreeItem(Session* session, QTreeWidgetItem* parent);
    ~SessionTreeItem();

    Session* session() const;
    SessionTreeItem* findChild(const QString& name) const;

    QVariant data(int column, int role) const;

    bool isAlerted() const;
    void setAlerted(bool alerted);

    bool isInactive() const;
    void setInactive(bool inactive);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

private:
    struct Private
    {
        bool alerted;
        bool inactive;
        bool highlighted;
        Session* session;
        QSet<SessionTreeItem*> alertedChildren;
        QSet<SessionTreeItem*> highlightedChildren;
    } d;
    friend class SessionTreeWidget;
};

#endif // SESSIONTREEITEM_H
