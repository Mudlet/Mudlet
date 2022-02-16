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

dlgMapLabel::dlgMapLabel(QWidget* pF) : QDialog(pF)
, fgColor(QColor(255, 255, 50, 255))
, bgColor(QColor(50, 50, 150, 100))
{
    setupUi(this);

    setWindowTitle(tr("Create label", "Create label dialog title"));

    connect(lineEdit_text, &QLineEdit::textChanged, this, [=](QString pText) {
        text = pText;
        emit updated();
    });
    connect(pushButton_bgColor, &QPushButton::released, this, &dlgMapLabel::pickBgColor);
    connect(pushButton_fgColor, &QPushButton::released, this, &dlgMapLabel::pickFgColor);
    connect(toolButton_fontPick, &QToolButton::released, this, &dlgMapLabel::pickFont);
    connect(pushButton_save, &QPushButton::released, this, &dlgMapLabel::save);
    connect(pushButton_cancel, &QPushButton::released, this, &dlgMapLabel::close);
    connect(comboBox_position, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){
        emit updated();
    });
    connect(this, &dlgMapLabel::updated, this, &dlgMapLabel::updateControls);

    font = QApplication::font();
    fontDialog = new QFontDialog(font, this);
    connect(fontDialog, &QFontDialog::currentFontChanged, this, [=](QFont pFont) {
        font = pFont;
        emit updated();
    });

    bgColorDialog = new QColorDialog(this);
    bgColorDialog->setWindowTitle(tr("Background color", "2D Mapper create label color dialog title"));
    bgColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(bgColorDialog, &QColorDialog::currentColorChanged, this, [=](QColor color) {
       bgColor = color;
       emit updated();
    });
    fgColorDialog = new QColorDialog(this);
    fgColorDialog->setWindowTitle(tr("Foreground color", "2D Mapper create label color dialog title"));
    fgColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(fgColorDialog, &QColorDialog::currentColorChanged, this, [=](QColor color) {
        fgColor = color;
        emit updated();
    });
    text = lineEdit_text->placeholderText();
    updateControls();
}

dlgMapLabel::~dlgMapLabel() {
    delete fontDialog;
    delete fgColorDialog;
    delete bgColorDialog;
}

void dlgMapLabel::pickFgColor()
{
    auto originalColor = QColor(fgColor);
    connect(fgColorDialog, &QColorDialog::rejected, this, [=]() {
        fgColor = originalColor;
        emit updated();
    });
    fgColorDialog->show();
    fgColorDialog->raise();
}

void dlgMapLabel::pickBgColor()
{
    auto originalColor = QColor(bgColor);
    connect(bgColorDialog, &QColorDialog::rejected, this, [=]() {
        bgColor = originalColor;
        emit updated();
    });
    bgColorDialog->show();
    bgColorDialog->raise();
}

void dlgMapLabel::pickFont()
{
    auto originalFont = QFont(font);
    connect(fontDialog, &QFontDialog::rejected, this, [=]() {
        font = originalFont;
        emit updated();
    });

    fontDialog->setCurrentFont(font);
    fontDialog->show();
    fontDialog->raise();
}

void dlgMapLabel::save() {
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
    return onTop;
}

void dlgMapLabel::updateControls() {
    lineEdit_font->setText(QString("%1, %2pt %3").arg(font.family(), QString::number(font.pointSize()), font.styleName()));
    pushButton_fgColor->setStyleSheet(BUTTON_STYLESHEET.arg(QString::number(fgColor.red()), QString::number(fgColor.green()), QString::number(fgColor.blue()), QString::number(fgColor.alpha())));
    pushButton_bgColor->setStyleSheet(BUTTON_STYLESHEET.arg(QString::number(bgColor.red()), QString::number(bgColor.green()), QString::number(bgColor.blue()), QString::number(bgColor.alpha())));
}