/***************************************************************************
 *   Copyright (C) 2022 by Piotr Wilczynski - delwing@gmail.com            *
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
#include "dlgMapLabel.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QFontDialog>
#include "post_guard.h"

static QString BUTTON_STYLESHEET = QStringLiteral("QPushButton { background-color: rgba(%1, %2, %3, %4); }");

dlgMapLabel::dlgMapLabel(QWidget* pParentWidget)
: QDialog(pParentWidget)
, fgColor(QColor(255, 255, 50, 255))
, bgColor(QColor(50, 50, 150, 100))
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Create label", "Create label dialog title"));

    connect(comboBox_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgMapLabel::slot_updateControlsVisibility);
    connect(comboBox_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgMapLabel::updated);
    connect(toolButton_imagePick, &QToolButton::released, this, &dlgMapLabel::slot_pickFile);
    connect(checkBox_stretchImage, &QCheckBox::stateChanged, this, &dlgMapLabel::updated);
    connect(lineEdit_text, &QLineEdit::textChanged, this, [&](const QString& pText) {
        text = pText;
        emit updated();
    });
    connect(pushButton_bgColor, &QPushButton::released, this, &dlgMapLabel::slot_pickBgColor);
    connect(pushButton_fgColor, &QPushButton::released, this, &dlgMapLabel::slot_pickFgColor);
    connect(toolButton_fontPick, &QToolButton::released, this, &dlgMapLabel::slot_pickFont);
    connect(pushButton_save, &QPushButton::released, this, &dlgMapLabel::slot_save);
    connect(pushButton_cancel, &QPushButton::released, this, &dlgMapLabel::close);
    connect(comboBox_position, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() { emit updated(); });
    connect(checkBox_scaling, &QCheckBox::stateChanged, this, [=]() { emit updated(); });
    connect(this, &dlgMapLabel::updated, this, &dlgMapLabel::slot_updateControls);

    font = QApplication::font();
    font.setStyle(QFont::StyleNormal);
    text = lineEdit_text->placeholderText();
    slot_updateControls();
    slot_updateControlsVisibility();
}

dlgMapLabel::~dlgMapLabel() {}

bool dlgMapLabel::isTextLabel()
{
    return comboBox_type->currentIndex() == 0;
}

QString dlgMapLabel::getImagePath()
{
    return imagePath;
}

void dlgMapLabel::slot_pickFgColor()
{
    fgColorDialog = new QColorDialog(this);
    fgColorDialog->setAttribute(Qt::WA_DeleteOnClose);
    fgColorDialog->setWindowTitle(tr("Foreground color", "2D mapper create label color dialog title"));
    fgColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(fgColorDialog, &QColorDialog::currentColorChanged, this, [&](const QColor& color) {
        fgColor = color;
        emit updated();
    });
    auto originalColor = QColor(fgColor);
    connect(fgColorDialog, &QColorDialog::rejected, this, [=]() {
        fgColor = originalColor;
        emit updated();
    });
    fgColorDialog->show();
    fgColorDialog->raise();
}

void dlgMapLabel::slot_pickBgColor()
{
    auto originalColor = QColor(bgColor);
    bgColorDialog = new QColorDialog(this);
    bgColorDialog->setAttribute(Qt::WA_DeleteOnClose);
    bgColorDialog->setWindowTitle(tr("Background color", "2D mapper create label color dialog title"));
    bgColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(bgColorDialog, &QColorDialog::currentColorChanged, this, [&](const QColor& color) {
        bgColor = color;
        emit updated();
    });
    connect(bgColorDialog, &QColorDialog::rejected, this, [=]() {
        bgColor = originalColor;
        emit updated();
    });
    bgColorDialog->show();
    bgColorDialog->raise();
}

void dlgMapLabel::slot_pickFont()
{
    auto originalFont = QFont(font);
    fontDialog = new QFontDialog(font, this);
    fontDialog->setAttribute(Qt::WA_DeleteOnClose);
    fontDialog->setWindowTitle(tr("Label font", "2D mapper create label font dialog title"));
    connect(fontDialog, &QFontDialog::currentFontChanged, this, [&](const QFont& pFont) {
        font = pFont;
        emit updated();
    });
    connect(fontDialog, &QFontDialog::rejected, this, [=]() {
        font = originalFont;
        emit updated();
    });

    fontDialog->setCurrentFont(font);
    fontDialog->show();
    fontDialog->raise();
}

void dlgMapLabel::slot_pickFile()
{
    imagePath = QFileDialog::getOpenFileName(nullptr, tr("Select image", "2D Mapper create label file dialog title"));
    emit updated();
}

void dlgMapLabel::slot_save()
{
    accept();
}

QString dlgMapLabel::getText()
{
    return text;
}

QColor& dlgMapLabel::getBgColor()
{
    return bgColor;
}

QColor& dlgMapLabel::getFgColor()
{
    return fgColor;
}

QFont& dlgMapLabel::getFont()
{
    return font;
}

bool dlgMapLabel::isOnTop()
{
    return comboBox_position->currentIndex() == 1;
}

bool dlgMapLabel::noScale()
{
    return !checkBox_scaling->isChecked();
}

bool dlgMapLabel::stretchImage()
{
    return checkBox_stretchImage->isChecked();
}

void dlgMapLabel::slot_updateControls()
{
    lineEdit_font->setText(QString("%1, %2pt %3").arg(font.family(), QString::number(font.pointSize()), font.styleName()));
    pushButton_fgColor->setStyleSheet(BUTTON_STYLESHEET.arg(QString::number(fgColor.red()), QString::number(fgColor.green()), QString::number(fgColor.blue()), QString::number(fgColor.alpha())));
    pushButton_bgColor->setStyleSheet(BUTTON_STYLESHEET.arg(QString::number(bgColor.red()), QString::number(bgColor.green()), QString::number(bgColor.blue()), QString::number(bgColor.alpha())));
    lineEdit_image->setText(imagePath);
}

void dlgMapLabel::slot_updateControlsVisibility()
{
    bool isText = isTextLabel();
    label_image->setVisible(!isText);
    lineEdit_image->setVisible(!isText);
    checkBox_stretchImage->setVisible(!isText);
    toolButton_imagePick->setVisible(!isText);
    label_text->setVisible(isText);
    lineEdit_text->setVisible(isText);
    label_font->setVisible(isText);
    lineEdit_font->setVisible(isText);
    toolButton_fontPick->setVisible(isText);
    pushButton_fgColor->setVisible(isText);
    label_fg->setVisible(isText);
}
