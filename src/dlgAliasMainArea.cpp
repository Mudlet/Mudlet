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


#include "dlgAliasMainArea.h"
#include "TrailingWhitespaceMarker.h"
#include "mudlet.h"

dlgAliasMainArea::dlgAliasMainArea(QWidget* pParentWidget)
: QWidget(pParentWidget)
{
    // init generated dialog
    setupUi(this);

    connect(lineEdit_alias_name, &QLineEdit::editingFinished, this, &dlgAliasMainArea::slot_editingNameFinished);
    connect(lineEdit_alias_pattern, &QLineEdit::textChanged, this, &dlgAliasMainArea::slot_changedPattern);

    if (mudlet::self()->smFirstLaunch) {
        //: This text is shown as placeholder in the pattern box when no real pattern was entered, yet.
        lineEdit_alias_pattern->setPlaceholderText(tr("for example, ^myalias$ to match 'myalias'"));
    }
}

void dlgAliasMainArea::trimName()
{
    lineEdit_alias_name->setText(lineEdit_alias_name->text().trimmed());
}

void dlgAliasMainArea::slot_editingNameFinished()
{
    trimName();
}
void dlgAliasMainArea::slot_changedPattern()
{
    markQLineEdit(lineEdit_alias_pattern);
}
