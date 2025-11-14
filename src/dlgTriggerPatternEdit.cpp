/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019, 2022 by Stephen Lyons - slysven@virginmedia.com   *
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

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QPlainTextEdit>
#include <QColor>
#include <QComboBox>
#include <QLineEdit>
#include <QPalette>
#include <QWidget>
#include <QAction>
#include <QDebug>

dlgTriggerPatternEdit::dlgTriggerPatternEdit(QWidget* pParentWidget)
: QWidget(pParentWidget)
{
    // init generated dialog
    setupUi(this);

    mDefaultPalette = palette();
    mDefaultPatternNumberPalette = label_patternNumber->palette();
    mDefaultPromptPalette = label_prompt->palette();
    mDefaultComboPalette = comboBox_patternType->palette();
    mDefaultSpinPalette = spinBox_lineSpacer->palette();
    mDefaultForegroundButtonPalette = pushButton_fgColor->palette();
    mDefaultBackgroundButtonPalette = pushButton_bgColor->palette();
    mDefaultPatternEditPalette = singleLineTextEdit_pattern->palette();
    if (auto* patternViewport = singleLineTextEdit_pattern->viewport()) {
        mDefaultPatternEditViewportPalette = patternViewport->palette();
        mDefaultPatternEditViewportAutoFillBackground = patternViewport->autoFillBackground();
    }

    // delay the connection so the pattern type is available for the slot
    connect(comboBox_patternType, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerPatternEdit::slot_triggerTypeComboBoxChanged, Qt::QueuedConnection);
}

void dlgTriggerPatternEdit::slot_triggerTypeComboBoxChanged(const int index)
{
    label_colorIcon->setPixmap(comboBox_patternType->itemIcon(index).pixmap(15, 15));

}

void dlgTriggerPatternEdit::applyThemePalette(const QPalette& editorPalette)
{
    const QColor baseColor = editorPalette.color(QPalette::Base);
    const QColor textColor = editorPalette.color(QPalette::Text);

    if (!baseColor.isValid() || !textColor.isValid()) {
        resetThemePalette();
        return;
    }

    setPalette(editorPalette);
    setAutoFillBackground(false);

    auto applyToWidget = [&](QWidget* widget) {
        if (!widget) {
            return;
        }

        widget->setPalette(editorPalette);

        if (auto* spinBox = qobject_cast<QAbstractSpinBox*>(widget)) {
            if (auto* lineEdit = spinBox->findChild<QLineEdit*>()) {
                lineEdit->setPalette(editorPalette);
            }
        }

        if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
            if (auto* view = comboBox->view()) {
                view->setPalette(editorPalette);
            }
        }

        if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
            button->setAutoFillBackground(false);
        }

        if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(widget)) {
            if (auto* viewport = plainTextEdit->viewport()) {
                viewport->setPalette(editorPalette);
                viewport->setAutoFillBackground(true);
            }
        } else if (auto* scrollArea = qobject_cast<QAbstractScrollArea*>(widget)) {
            if (auto* viewport = scrollArea->viewport()) {
                viewport->setPalette(editorPalette);
                viewport->setAutoFillBackground(true);
            }
        }
    };

    applyToWidget(label_colorIcon);
    applyToWidget(label_patternNumber);
    applyToWidget(label_prompt);
    applyToWidget(comboBox_patternType);
    applyToWidget(spinBox_lineSpacer);
    applyToWidget(pushButton_fgColor);
    applyToWidget(pushButton_bgColor);
    applyToWidget(singleLineTextEdit_pattern);
}

void dlgTriggerPatternEdit::resetThemePalette()
{
    setAutoFillBackground(false);
    setPalette(mDefaultPalette);

    label_patternNumber->setPalette(mDefaultPatternNumberPalette);
    label_prompt->setPalette(mDefaultPromptPalette);
    comboBox_patternType->setPalette(mDefaultComboPalette);
    spinBox_lineSpacer->setPalette(mDefaultSpinPalette);
    pushButton_fgColor->setPalette(mDefaultForegroundButtonPalette);
    pushButton_bgColor->setPalette(mDefaultBackgroundButtonPalette);
    singleLineTextEdit_pattern->setPalette(mDefaultPatternEditPalette);

    if (auto* patternViewport = singleLineTextEdit_pattern->viewport()) {
        patternViewport->setPalette(mDefaultPatternEditViewportPalette);
        patternViewport->setAutoFillBackground(mDefaultPatternEditViewportAutoFillBackground);
    }

    if (auto* spinBoxLineEdit = spinBox_lineSpacer->findChild<QLineEdit*>()) {
        spinBoxLineEdit->setPalette(mDefaultSpinPalette);
    }

    if (auto* comboBoxView = comboBox_patternType->view()) {
        comboBoxView->setPalette(mDefaultComboPalette);
    }

    pushButton_fgColor->setAutoFillBackground(false);
    pushButton_bgColor->setAutoFillBackground(false);
}
