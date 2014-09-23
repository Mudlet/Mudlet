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

#include "lineeditor.h"
#include "completer.h"
#include <QStyleFactory>
#include <QShortcut>
#include <QStyle>

LineEditor::LineEditor(QWidget* parent) : HistoryLineEdit(parent)
{
    d.completer = new Completer(this);
    d.completer->setWidget(this);
    d.completer->setLineEdit(this);

    setAttribute(Qt::WA_MacShowFocusRect, false);

    // a workaround for a bug in the Oxygen style (style animation eats up all cpu)
    if (style()->objectName() == "oxygen") {
        QStringList keys = QStringList() << "fusion" << "plastique" << "cleanlooks" << "windows";
        while (!keys.isEmpty()) {
            QString key = keys.takeFirst();
            if (QStyleFactory::keys().contains(key)) {
                setStyle(QStyleFactory::create(key));
                break;
            }
        }
    }

    QShortcut* shortcut = new QShortcut(Qt::Key_Tab, this);
    connect(shortcut, SIGNAL(activated()), d.completer, SLOT(onTabPressed()));

    setButtonVisible(Left, true);
    setAutoHideButton(Left, true);
    setButtonPixmap(Left, QPixmap(":/resources/icons/buttons/tab.png"));
    connect(this, SIGNAL(leftButtonClicked()), d.completer, SLOT(onTabPressed()));

    setButtonVisible(Right, true);
    setAutoHideButton(Right, true);
    setButtonPixmap(Right, QPixmap(":/resources/icons/buttons/return.png"));
    connect(this, SIGNAL(rightButtonClicked()), this, SLOT(onSend()));

    connect(this, SIGNAL(returnPressed()), this, SLOT(onSend()));
    connect(this, SIGNAL(textChanged(QString)), this, SIGNAL(typed(QString)));
}

Completer* LineEditor::completer() const
{
    return d.completer;
}

void LineEditor::onSend()
{
    const QString input = text();
    if (!input.isEmpty())
    {
        clear();
        emit send(input);
    }
}
