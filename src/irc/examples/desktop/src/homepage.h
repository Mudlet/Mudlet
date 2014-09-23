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

#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
class QLabel;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    HomePage(QWidget* parent = 0);

signals:
    void connectRequested();

protected:
    void paintEvent(QPaintEvent* event);

private:
    QWidget* createBody(QWidget* parent = 0) const;

    QLabel* header;
    QLabel* slogan;
    QLabel* footer;
    QPixmap bg;
};

#endif // HOMEPAGE_H
