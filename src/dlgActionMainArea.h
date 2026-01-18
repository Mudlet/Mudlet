#ifndef MUDLET_DLGACTIONMAINAREA_H
#define MUDLET_DLGACTIONMAINAREA_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "ui_actions_main_area.h"


class dlgActionMainArea : public QWidget, public Ui::actions_main_area
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgActionMainArea)
    explicit dlgActionMainArea(QWidget*);

    // public function allow to trim even when QLineEdit::editingFinished()
    // is not raised. Example: When the user saves without leaving the LineEdit
    void trimName();

private slots:
    void slot_editingNameFinished();
};

#endif // MUDLET_DLGACTIONMAINAREA_H
