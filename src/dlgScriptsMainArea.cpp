/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "dlgScriptsMainArea.h"


dlgScriptsMainArea::dlgScriptsMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    connect(lineEdit_script_name, &QLineEdit::editingFinished, this, &dlgScriptsMainArea::slot_editing_name_finished);
    connect(lineEdit_script_event_handler_entry, &QLineEdit::editingFinished, this, &dlgScriptsMainArea::slot_editing_event_name_finished);
}

void dlgScriptsMainArea::trimName()
{
    lineEdit_script_name->setText(lineEdit_script_name->text().trimmed());
}

void dlgScriptsMainArea::trimEventHandlerName()
{
    lineEdit_script_event_handler_entry->setText(lineEdit_script_event_handler_entry->text().trimmed());
}

void dlgScriptsMainArea::slot_editing_name_finished()
{
    trimName();
}

void dlgScriptsMainArea::slot_editing_event_name_finished()
{
    trimEventHandlerName();
}
