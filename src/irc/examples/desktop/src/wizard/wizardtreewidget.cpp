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

#include "wizardtreewidget.h"

WizardTreeWidget::WizardTreeWidget(QWidget* parent) : QTreeWidget(parent)
{
}

bool WizardTreeWidget::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    QModelIndex sibling = index.sibling(index.row(), 1);
    return QTreeWidget::edit(sibling, trigger, event);
}
