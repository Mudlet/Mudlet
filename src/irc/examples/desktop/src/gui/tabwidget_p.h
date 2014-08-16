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

#ifndef TABWIDGET_P_H
#define TABWIDGET_P_H

#include <QTabBar>

class QShortcut;

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    TabBar(QWidget* parent = 0);

    enum Navigation { Next, Previous, NextUnread, PreviousUnread };
    QKeySequence navigationShortcut(Navigation navigation) const;
    void setNavigationShortcut(Navigation navigation, const QKeySequence& shortcut);

    QSize minimumSizeHint() const;

signals:
    void menuRequested(int index, const QPoint& pos);

protected:
    void changeEvent(QEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    void wheelEvent(QWheelEvent* event);

private:
    QShortcut* prevShortcut;
    QShortcut* nextShortcut;
    QShortcut* prevUnreadShortcut;
    QShortcut* nextUnreadShortcut;
};

#endif // TABWIDGET_P_H
