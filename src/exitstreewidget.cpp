/***************************************************************************
 *   Copyright (C) 2012 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "exitstreewidget.h"

#include "pre_guard.h"
#include <QtEvents>
#include "post_guard.h"


ExitsTreeWidget::ExitsTreeWidget(QWidget* pW) : QTreeWidget(pW)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setUniformRowHeights(true);
}

void ExitsTreeWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        closePersistentEditor(currentItem(), 1);
        closePersistentEditor(currentItem(), 2);
    }
    if (event->key() == Qt::Key_Delete && hasFocus()) {
        QList<QTreeWidgetItem*> selection = selectedItems();
        foreach (QTreeWidgetItem* item, selection) {
            takeTopLevelItem(indexOfTopLevelItem(item));
        }
    }
}
