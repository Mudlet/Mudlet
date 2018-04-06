/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgVarsMainArea.h"

#include "pre_guard.h"
#include <QListWidgetItem>
#include "post_guard.h"

dlgVarsMainArea::dlgVarsMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Modify the normal QComboBoxes with customised data models that impliment
    // https://stackoverflow.com/a/21376774/4805858 so that individual entries
    // can be "disabled":
    // Key type widget:

    // Magic - part 1 set up a replacement data model:
    QListWidget *contents = new QListWidget(comboBox_variable_key_type);
    contents->hide();
    comboBox_variable_key_type->setModel(contents->model());

    // Now populate the widget - throw away stuff entered from form design
    // before substitute model was attached:
    comboBox_variable_key_type->clear();
    comboBox_variable_key_type->insertItem(0, tr("Auto-Type"), -1); // LUA_TNONE
    comboBox_variable_key_type->insertItem(1, tr("key (string)"), 4);  // LUA_TSTRING
    comboBox_variable_key_type->insertItem(2, tr("index (integer number)"), 3);  // LUA_TNUMBER
    comboBox_variable_key_type->insertItem(3, tr("table (use \"Add Group\" to create)"), 5);  // LUA_TTABLE
    comboBox_variable_key_type->insertItem(4, tr("function (cannot create from GUI)"), 6);  // LUA_TFUNCTION

    // Magic - part 2 use the features of the substitute data model to disable
    // the required entries - they can still be set programmatically for display
    // purposes:
    // Disable table type:
    QListWidgetItem *item = contents->item(3);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

    // Disable function type:
    item = contents->item(4);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);


    // Value type widget:
    // Magic - part 1 set up a replacement data model:
    contents = new QListWidget(comboBox_variable_value_type);
    contents->hide();
    comboBox_variable_value_type->setModel(contents->model());

    // Now populate the widget - throw away stuff entered from form design
    // before substitute model was attached:
    comboBox_variable_value_type->clear();
    comboBox_variable_value_type->insertItem(0, tr("Auto-Type"), -1); // LUA_TNONE
    comboBox_variable_value_type->insertItem(1, tr("string"), 4);  // LUA_TSTRING
    comboBox_variable_value_type->insertItem(2, tr("number"), 3);  // LUA_TNUMBER
    comboBox_variable_value_type->insertItem(3, tr("boolean"), 1);  // LUA_TBOOLEAN
    comboBox_variable_value_type->insertItem(4, tr("table"), 5);  // LUA_TTABLE
    comboBox_variable_value_type->insertItem(5, tr("function"), 6);  // LUA_TFUNCTION

    // Magic - part 2 use the features of the substitute data model:
    // Disable function type:
    item = contents->item(5);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
}
