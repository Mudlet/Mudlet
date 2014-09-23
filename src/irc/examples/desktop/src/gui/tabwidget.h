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

#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include <QColor>
#include <QList>
#include <QHash>

class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    TabWidget(QWidget* parent = 0);

    QTabBar* tabBar() const;
    QSize sizeHint() const;

    enum TabRole { Active, Inactive, Alert, Highlight };

    QColor tabTextColor(TabRole role) const;
    void setTabTextColor(TabRole role, const QColor& color);

    bool isTabInactive(int index = -1);
    void setTabInactive(int index, bool inactive);

    bool hasTabAlert(int index = -1);
    void setTabAlert(int index, bool alert);

    bool hasTabHighlight(int index = -1) const;
    void setTabHighlight(int index, bool highlight);

public slots:
    void moveToNextTab();
    void moveToPrevTab();
    void moveToNextUnreadTab();
    void moveToPrevUnreadTab();

signals:
    void newTabRequested();
    void tabMenuRequested(int index, const QPoint& pos);
    void alertStatusChanged(bool alerted);
    void highlightStatusChanged(bool highlighted);

protected:
    void tabInserted(int index);
    void tabRemoved(int index);

private slots:
    void tabChanged(int index);
    void alertTimeout();
    void colorizeTab(int index);
    void updateTabColors();

private:
    struct TabWidgetData
    {
        int previous;
        bool updatingColors;
        QColor currentAlertColor;
        QList<int> inactiveIndexes;
        QList<int> alertIndexes;
        QList<int> highlightIndexes;
        QHash<TabRole, QColor> colors;
    } d;
};

#endif // TABWIDGET_H
