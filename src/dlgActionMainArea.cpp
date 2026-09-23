/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022, 2026 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "dlgActionMainArea.h"


dlgActionMainArea::dlgActionMainArea(QWidget* pParentWidget)
: QWidget(pParentWidget)
{
    // init generated dialog
    setupUi(this);

    connect(lineEdit_action_name, &QLineEdit::editingFinished, this, &dlgActionMainArea::slot_editingNameFinished);
    connect(spinBox_action_bar_columns, &QSpinBox::valueChanged, this, &dlgActionMainArea::slot_setMaximumValueForOffset);
    connect(comboBox_action_bar_orientation, &QComboBox::currentIndexChanged, this, &dlgActionMainArea::slot_setColumnsOrRowsCountText);
    // Hide until we can resurrect icons on menus and buttons:
    label_action_icon->hide();
    lineEdit_action_icon->hide();
}

void dlgActionMainArea::trimName()
{
    lineEdit_action_name->setText(lineEdit_action_name->text().trimmed());
}

void dlgActionMainArea::slot_editingNameFinished()
{
    trimName();
}

void dlgActionMainArea::slot_setMaximumValueForOffset(const int value)
{
    // Disable or hide the offset control if required:
    if (value > 1) {
        spinBox_action_bar_offsetToFirstButton->setMaximum(value - 1);
        if (spinBox_action_bar_offsetToFirstButton->value() >= value) {
            spinBox_action_bar_offsetToFirstButton->setValue(spinBox_action_bar_offsetToFirstButton->maximum());
        }
        spinBox_action_bar_offsetToFirstButton->setEnabled(true);
        label_action_bar_offsetToFirstButton->setEnabled(true);
        spinBox_action_bar_offsetToFirstButton->setVisible(true);
        label_action_bar_offsetToFirstButton->setVisible(true);
    } else {
        spinBox_action_bar_offsetToFirstButton->setMaximum(0);
        spinBox_action_bar_offsetToFirstButton->setValue(0);
        if (value == 1) {
            spinBox_action_bar_offsetToFirstButton->setEnabled(false);
            label_action_bar_offsetToFirstButton->setEnabled(false);
            spinBox_action_bar_offsetToFirstButton->setVisible(true);
            label_action_bar_offsetToFirstButton->setVisible(true);
        } else {
            /* A zero value has previously been allowed and it is possible that
             * that value was intended to trigger the previous
             * "mUseCustomLayout".*/
            spinBox_action_bar_offsetToFirstButton->setEnabled(false);
            label_action_bar_offsetToFirstButton->setEnabled(false);
            spinBox_action_bar_offsetToFirstButton->setVisible(false);
            label_action_bar_offsetToFirstButton->setVisible(false);
        }
    }
}

// index: 0 = horizontalm 1 = vertical
void dlgActionMainArea::slot_setColumnsOrRowsCountText(const int index)
{
    if (index > 0) {
        //: A toolbar is being set to vertical orientation - so multiple rows of this number of columns
        label_action_bar_columns->setText(tr("Number of columns:"));
    } else {
        //: A toolbar is being set to horizontal orientation - so multiple columns of this number of rows
        label_action_bar_columns->setText(tr("Number of rows:"));
    }
}
