/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgTriggerPatternEdit.h"
#include "TTrigger.h"

#include "pre_guard.h"
#include <QAction>
#include <QDebug>
#include "post_guard.h"

dlgTriggerPatternEdit::dlgTriggerPatternEdit(QWidget* pF)
: QWidget(pF)
, mRow()
{
    // init generated dialog
    setupUi(this);
    // delay the connection so the pattern type is available for the slot
    connect(comboBox_patternType, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerPatternEdit::slot_triggerTypeComboBoxChanged, Qt::QueuedConnection);
}

void dlgTriggerPatternEdit::slot_triggerTypeComboBoxChanged(const int index)
{
    label_colorIcon->setPixmap(comboBox_patternType->itemIcon(index).pixmap(15, 15));

    const bool firstRow = comboBox_patternType->itemData(0).toInt() == 0;
    if (!firstRow) {
        return;
    }

    switch (comboBox_patternType->currentIndex()) {
    case REGEX_SUBSTRING:
        lineEdit_pattern->setPlaceholderText(tr("Text to find (anywhere in the game output)"));
        break;
    case REGEX_PERL:
        lineEdit_pattern->setPlaceholderText(tr("Text to find (as a regular expression pattern)"));
        break;
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
        lineEdit_pattern->setPlaceholderText(tr("Text to find (from beginning of the line)"));
        break;
    case REGEX_EXACT_MATCH:
        lineEdit_pattern->setPlaceholderText(tr("Exact line to match"));
        break;
    case REGEX_LUA_CODE:
        lineEdit_pattern->setPlaceholderText(tr("Lua code to run (return true to match)"));
        break;
    }
}
