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

#ifndef WIZARDTREEWIDGET_H
#define WIZARDTREEWIDGET_H

#include <QTreeWidget>

class WizardTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    WizardTreeWidget(QWidget* parent = 0);

protected:
    virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event);
};

#endif // WIZARDTREEWIDGET_H
