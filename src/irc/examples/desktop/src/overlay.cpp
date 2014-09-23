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

#include "overlay.h"
#include <QResizeEvent>
#include <QMovie>

Overlay::Overlay(QWidget* parent) : QLabel(parent)
{
    d.button = 0;

    setVisible(false);
    setEnabled(false);
    setAutoFillBackground(true);
    setAlignment(Qt::AlignCenter);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    QPalette pal = palette();
    QColor col = pal.color(QPalette::Window);
    col.setAlpha(100);
    pal.setColor(QPalette::Window, col);
    setPalette(pal);

    parent->installEventFilter(this);
    relayout();
}

bool Overlay::isBusy() const
{
    return movie();
}

void Overlay::setBusy(bool busy)
{
    if (busy)
    {
        setMovie(new QMovie(":/resources/ajax-loader.gif", QByteArray(), this));
        movie()->start();
    }
    else
    {
        delete movie();
    }
}

bool Overlay::hasRefresh() const
{
    return d.button;
}

void Overlay::setRefresh(bool enabled)
{
    if (enabled)
    {
        if (!d.button)
        {
            d.button = new QToolButton(parentWidget());
            d.button->setObjectName("reconnectButton");
            d.button->setFixedSize(32, 32);
            connect(d.button, SIGNAL(clicked()), this, SIGNAL(refresh()));
            relayout();
        }
        d.button->show();
    }
    else
    {
        if (d.button)
            d.button->deleteLater();
        d.button = 0;
    }
}

bool Overlay::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);
    if (event->type() == QEvent::Resize)
        relayout();
    return false;
}

void Overlay::relayout()
{
    resize(parentWidget()->size());
    if (d.button)
        d.button->move(rect().center() - d.button->rect().center());
}
