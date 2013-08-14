/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   <email Chris>                                                         *
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

#include <QWidget>
#include <QMenu>


#include "dlgVarsMainArea.h"

dlgVarsMainArea::dlgVarsMainArea(QWidget * pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);
    key_type->setItemData(0, -1, Qt::UserRole);
    key_type->setItemData(1, 4, Qt::UserRole);
    key_type->setItemData(2, 3, Qt::UserRole);
    var_type->setItemData(0, -1, Qt::UserRole);
    var_type->setItemData(1, 4, Qt::UserRole);
    var_type->setItemData(2, 3, Qt::UserRole);
    var_type->setItemData(3, 1, Qt::UserRole);
    var_type->setItemData(4, 5, Qt::UserRole);
    /*var_type->setText("Variable Name Type");
    var_type->setPopupMode(QToolButton::InstantPopup);
    QMenu * varTypeMenu = new QMenu(var_type);
    varTypeMenu->addAction("String(default)");
    varTypeMenu->addAction("Integer");
    var_type->setMenu(varTypeMenu);
    value_type->setText("Variable Value Type");
    value_type->setPopupMode(QToolButton::InstantPopup);
    QMenu * valueTypeMenu = new QMenu(value_type);
    valueTypeMenu->addAction("String(default)");
    valueTypeMenu->addAction("Integer");
    valueTypeMenu->addAction("Boolean");
    value_type->setMenu(valueTypeMenu);
    hideVariable->setText("Hidden Variable");*/
}

