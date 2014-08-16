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

#include "generalwizardpage.h"

GeneralWizardPage::GeneralWizardPage(QWidget* parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/oxygen/64x64/actions/configure.png"));
}

QFont GeneralWizardPage::font() const
{
    QFont font = ui.fontComboBox->currentFont();
    font.setPointSize(ui.sizeSpinBox->value());
    return font;
}

void GeneralWizardPage::setFont(const QFont& font)
{
    ui.fontComboBox->setCurrentFont(font);
    ui.sizeSpinBox->setValue(font.pointSize());
}

QString GeneralWizardPage::layout() const
{
    return ui.layoutComboBox->currentText().toLower();
}

void GeneralWizardPage::setLayout(const QString& layout)
{
    ui.layoutComboBox->setCurrentIndex(ui.layoutComboBox->findText(layout, Qt::MatchFixedString));
}

QString GeneralWizardPage::language() const
{
    // TODO
    return QString();
}

void GeneralWizardPage::setLanguage(const QString& language)
{
    // TODO
    Q_UNUSED(language);
}

int GeneralWizardPage::maxBlockCount() const
{
    return ui.blockSpinBox->value();
}

void GeneralWizardPage::setMaxBlockCount(int count)
{
    ui.blockSpinBox->setValue(count);
}

bool GeneralWizardPage::timeStamp() const
{
    return ui.timeStampCheckBox->isChecked();
}

void GeneralWizardPage::setTimeStamp(bool timeStamp)
{
    ui.timeStampCheckBox->setChecked(timeStamp);
}

bool GeneralWizardPage::stripNicks() const
{
    return ui.stripNicksCheckBox->isChecked();
}

void GeneralWizardPage::setStripNicks(bool strip)
{
    ui.stripNicksCheckBox->setChecked(strip);
}
