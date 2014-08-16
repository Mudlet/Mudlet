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

#ifndef OVERLAY_H
#define OVERLAY_H

#include <QLabel>
#include <QToolButton>

class Overlay : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy WRITE setBusy)
    Q_PROPERTY(bool refresh READ hasRefresh WRITE setRefresh)

public:
    explicit Overlay(QWidget* parent);

    bool isBusy() const;
    void setBusy(bool busy);

    bool hasRefresh() const;
    void setRefresh(bool enabled);

signals:
    void refresh();

protected:
    bool eventFilter(QObject* object, QEvent* event);

private slots:
    void relayout();

private:
    struct Private
    {
        QToolButton* button;
    } d;
};

#endif // OVERLAY_H
