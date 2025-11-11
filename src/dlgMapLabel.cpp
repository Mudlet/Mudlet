/***************************************************************************
 *   Copyright (C) 2022 by Piotr Wilczynski - delwing@gmail.com            *
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

#include "dlgMapLabel.h"
#include "mudlet.h"
#include "utils.h"
#include <QSettings>

static QString BUTTON_STYLESHEET = qsl("QPushButton { background-color: rgba(%1, %2, %3, %4); }");

dlgMapLabel::dlgMapLabel(QWidget* pParentWidget)
: QDialog(pParentWidget)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    //: Create label dialog title
    setWindowTitle(tr("Create label"));

    connect(comboBox_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgMapLabel::slot_updateControlsVisibility);
    connect(comboBox_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgMapLabel::updated);
    connect(toolButton_imagePick, &QToolButton::released, this, &dlgMapLabel::slot_pickFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(checkBox_stretchImage, &QCheckBox::checkStateChanged, this, &dlgMapLabel::updated);
#else
    connect(checkBox_stretchImage, &QCheckBox::stateChanged, this, &dlgMapLabel::updated);
#endif
    connect(plainTextEdit_labelText, &QPlainTextEdit::textChanged, this, [&]() {
        text = plainTextEdit_labelText->toPlainText();
        emit updated();
    });
    connect(pushButton_bgColor, &QPushButton::released, this, &dlgMapLabel::slot_pickBgColor);
    connect(pushButton_fgColor, &QPushButton::released, this, &dlgMapLabel::slot_pickFgColor);
    connect(pushButton_outlineColor, &QPushButton::released, this, &dlgMapLabel::slot_pickOutlineColor);
    connect(toolButton_fontPick, &QToolButton::released, this, &dlgMapLabel::slot_pickFont);
    connect(pushButton_save, &QPushButton::released, this, &dlgMapLabel::slot_save);
    connect(pushButton_cancel, &QPushButton::released, this, &dlgMapLabel::close);
    connect(comboBox_position, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgMapLabel::updated);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(checkBox_scaling, &QCheckBox::checkStateChanged, this, &dlgMapLabel::updated);
#else
    connect(checkBox_scaling, &QCheckBox::stateChanged, this, &dlgMapLabel::updated);
#endif
    connect(this, &dlgMapLabel::updated, this, &dlgMapLabel::slot_updateControls);

    font = QApplication::font();
    font.setStyle(QFont::StyleNormal);
    text = plainTextEdit_labelText->placeholderText();

    QSettings& settings = *mudlet::getQSettings();
    fgColor = settings.value("fgColorDialogMapLabel", fgColor).value<QColor>();
    bgColor = settings.value("bgColorDialogMapLabel", bgColor).value<QColor>();
    outlineColor = settings.value("outlineColorDialogMapLabel", outlineColor).value<QColor>();
    slot_updateControls();
    slot_updateControlsVisibility();
}

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
    QSettings& settings = *mudlet::getQSettings();
    fgColorDialog = new QColorDialog(this);
    fgColorDialog->setCurrentColor(settings.value("fgColorDialogMapLabel", fgColor).value<QColor>());
    fgColorDialog->setAttribute(Qt::WA_DeleteOnClose);
    //: 2D mapper create label color dialog title
    fgColorDialog->setWindowTitle(tr("Foreground color"));
    fgColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(fgColorDialog, &QColorDialog::currentColorChanged, this, [&](const QColor& color) {
        fgColor = color;
        settings.setValue("fgColorDialogMapLabel", fgColor);
        emit updated();
    });
    auto originalColor = QColor(fgColor);
    connect(fgColorDialog, &QColorDialog::rejected, this, [this, originalColor]() {
        fgColor = originalColor;
        emit updated();
    });
    fgColorDialog->show();
    fgColorDialog->raise();
}

void dlgMapLabel::slot_pickBgColor()
{
    QSettings& settings = *mudlet::getQSettings();
    bgColorDialog = new QColorDialog(this);
    bgColorDialog->setCurrentColor(settings.value("bgColorDialogMapLabel", bgColor).value<QColor>());
    bgColorDialog->setAttribute(Qt::WA_DeleteOnClose);
    //: 2D mapper create label color dialog title
    bgColorDialog->setWindowTitle(tr("Background color"));
    bgColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(bgColorDialog, &QColorDialog::currentColorChanged, this, [&](const QColor& color) {
        bgColor = color;
        settings.setValue("bgColorDialogMapLabel", bgColor);
        emit updated();
    });
    auto originalColor = QColor(bgColor);
    connect(bgColorDialog, &QColorDialog::rejected, this, [this, originalColor]() {
        bgColor = originalColor;
        emit updated();
    });
    bgColorDialog->show();
    bgColorDialog->raise();
}

void dlgMapLabel::slot_pickOutlineColor()
{
    QSettings& settings = *mudlet::getQSettings();
    outlineColorDialog = new QColorDialog(this);
    outlineColorDialog->setCurrentColor(settings.value("outlineColorDialogMapLabel", outlineColor).value<QColor>());
    outlineColorDialog->setAttribute(Qt::WA_DeleteOnClose);
    //: 2D mapper create label color dialog title
    outlineColorDialog->setWindowTitle(tr("Text outline color"));
    outlineColorDialog->setOption(QColorDialog::ShowAlphaChannel);
    connect(outlineColorDialog, &QColorDialog::currentColorChanged, this, [&](const QColor& color) {
        outlineColor = color;
        settings.setValue("outlineColorDialogMapLabel", outlineColor);
        emit updated();
    });
    auto originalColor = QColor(outlineColor);
    connect(outlineColorDialog, &QColorDialog::rejected, this, [this, originalColor]() {
        outlineColor = originalColor;
        emit updated();
    });
    outlineColorDialog->show();
    outlineColorDialog->raise();
}

void dlgMapLabel::slot_pickFont()
{
    auto originalFont = QFont(font);
    fontDialog = new QFontDialog(font, this);
    fontDialog->setAttribute(Qt::WA_DeleteOnClose);
    //: 2D mapper create label font dialog title
    fontDialog->setWindowTitle(tr("Label font"));
    connect(fontDialog, &QFontDialog::currentFontChanged, this, [&](const QFont& pFont) {
        font = pFont;
        emit updated();
    });
    connect(fontDialog, &QFontDialog::rejected, this, [this, originalFont]() {
        font = originalFont;
        emit updated();
    });

    fontDialog->setCurrentFont(font);
    fontDialog->show();
    fontDialog->raise();
}

void dlgMapLabel::slot_pickFile()
{
    //: 2D Mapper create label file dialog title

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    imagePath = QFileDialog::getOpenFileName(nullptr, tr("Select image"), lastDir);

    if (imagePath.isEmpty()) {
        return;
    }

    emit updated();
    lastDir = QFileInfo(imagePath).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);
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

QColor& dlgMapLabel::getOutlineColor()
{
    return outlineColor;
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
    pushButton_outlineColor->setStyleSheet(BUTTON_STYLESHEET.arg(QString::number(outlineColor.red()), QString::number(outlineColor.green()), QString::number(outlineColor.blue()), QString::number(outlineColor.alpha())));
    lineEdit_image->setText(imagePath);
}

void dlgMapLabel::slot_updateControlsVisibility()
{
    const bool isText = isTextLabel();
    label_image->setVisible(!isText);
    lineEdit_image->setVisible(!isText);
    checkBox_stretchImage->setVisible(!isText);
    toolButton_imagePick->setVisible(!isText);
    label_text->setVisible(isText);
    plainTextEdit_labelText->setVisible(isText);
    label_font->setVisible(isText);
    lineEdit_font->setVisible(isText);
    toolButton_fontPick->setVisible(isText);
    pushButton_fgColor->setVisible(isText);
    label_fg->setVisible(isText);
    pushButton_outlineColor->setVisible(isText);
    label_outline->setVisible(isText);
}
