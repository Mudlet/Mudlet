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

#include "toolbar.h"
#include <QLineEdit>
#include <QAction>
#include <QIcon>

static const int VERTICAL_MARGIN = 1; // matches qlineedit_p.cpp

ToolBar::ToolBar(QWidget* parent) : QToolBar(parent)
{
    setIconSize(QSize(12, 12));
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    addAction(QIcon(":/resources/iconmonstr/about.png"), "", this, SIGNAL(aboutTriggered()))->setToolTip(tr("About"));
    addAction(QIcon(":/resources/iconmonstr/settings.png"), "", this, SIGNAL(settingsTriggered()))->setToolTip(tr("Settings"));

    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addWidget(spacer);

    addAction(QIcon(":/resources/iconmonstr/connect.png"), "", this, SIGNAL(connectTriggered()))->setToolTip(tr("Connect"));
    addAction(QIcon(":/resources/iconmonstr/new-view.png"), "", this, SIGNAL(joinTriggered()))->setToolTip(tr("Add view"));

    QLineEdit lineEdit;
    lineEdit.setStyleSheet("QLineEdit { border: 1px solid transparent; }");
    setFixedHeight(lineEdit.sizeHint().height());
}
