/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
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


#include "dlgRoomSymbol.h"
#include "Host.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QPainter>
#include "post_guard.h"

dlgRoomSymbol::dlgRoomSymbol(Host* pHost, QWidget* pParentWidget) : QDialog(pParentWidget), mpHost(pHost)
{
    // init generated dialog
    setupUi(this);

    connect(lineEdit_roomSymbol, &QLineEdit::textChanged, this, &dlgRoomSymbol::updatePreview);
    connect(pushButton_roomSymbolColor, &QAbstractButton::released, this, &dlgRoomSymbol::openColorSelector);
    connect(pushButton_reset, &QAbstractButton::released, this, &dlgRoomSymbol::resetColor);
    connect(comboBox_roomSymbol, &QComboBox::currentTextChanged, this, &dlgRoomSymbol::updatePreview);

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgRoomSymbol::init(QHash<QString, int>& pSymbols, QSet<TRoom*>& pRooms)
{
    mpSymbols = pSymbols;
    mpRooms = pRooms;
    if (mpSymbols.size() <= 1) {
        lineEdit_roomSymbol->setText(!mpSymbols.isEmpty() ? mpSymbols.keys().first() : QString());
        comboBox_roomSymbol->hide();
    } else {
        comboBox_roomSymbol->addItems(getComboBoxItems());
        lineEdit_roomSymbol->hide();
    }
    initInstructionLabel();
    if (!pRooms.isEmpty()) {
        auto pRoom = *(pRooms.begin());
        if (pRoom) {
            auto firstRoomId = pRoom->getId();
            selectedColor = pRoom->mSymbolColor;
            previewColor = pRoom->mSymbolColor;
            roomColor = mpHost->mpMap->getColor(firstRoomId);
        }
    }
    updatePreview();
}

void dlgRoomSymbol::initInstructionLabel()
{
    if (mpSymbols.empty()) {
        label_instructions->hide();
        return;
    }

    QString instructions;
    if (mpSymbols.size() == 1) {
        if (mpRooms.size() > 1) {
            instructions = tr("The only used symbol is \"%1\" in one or\n"
                              "more of the selected %n room(s), delete this to\n"
                              "clear it from all selected rooms or replace\n"
                              "with a new symbol to use for all the rooms:",
                              // Intentional comment to separate arguments!
                              "This is for when applying a new room symbol to one or more rooms "
                              "and some have the SAME symbol (others may have none) at present, "
                              "%n is the total number of rooms involved and is at least two. "
                              "Use line feeds to format text into a reasonable rectangle.",
                              mpRooms.size()).arg(mpSymbols.keys().first());
        } else {
            instructions = tr("The symbol is \"%1\" in the selected room,\n"
                              "delete this to clear the symbol or replace\n"
                              "it with a new symbol for this room:",
                              // Intentional comment to separate arguments!
                              "This is for when applying a new room symbol to one room. "
                              "Use line feeds to format text into a reasonable rectangle.")
                                   .arg(mpSymbols.keys().first());
        }
    } else {
        instructions = tr("Choose:\n"
                          " • an existing symbol from the list below (sorted by most commonly used first)\n"
                          " • enter one or more graphemes (\"visible characters\") as a new symbol\n"
                          " • enter a space to clear any existing symbols\n"
                          "for all of the %n selected room(s):",
                          // Intentional comment to separate arguments!
                          "Use line feeds to format text into a reasonable rectangle if needed, "
                          "%n is the number of rooms involved.", mpRooms.size());
    }
    label_instructions->setText(instructions);
}

QStringList dlgRoomSymbol::getComboBoxItems()
{
    QHashIterator<QString, int> itSymbolUsed(mpSymbols);
    QSet<int> symbolCountsSet;
    while (itSymbolUsed.hasNext()) {
        itSymbolUsed.next();
        symbolCountsSet.insert(itSymbolUsed.value());
    }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QList<int> symbolCountsList{symbolCountsSet.begin(), symbolCountsSet.end()};
#else
    QList<int> symbolCountsList{symbolCountsSet.toList()};
#endif
    if (symbolCountsList.size() > 1) {
        std::sort(symbolCountsList.begin(), symbolCountsList.end());
    }
    QStringList displayStrings;
    for (int i = symbolCountsList.size() - 1; i >= 0; --i) {
        itSymbolUsed.toFront();
        while (itSymbolUsed.hasNext()) {
            itSymbolUsed.next();
            if (itSymbolUsed.value() == symbolCountsList.at(i)) {
                displayStrings.append(tr("%1 {count:%2}",
                                         // Intentional comment to separate arguments
                                         "Everything after the first parameter (the '%1') will be removed by processing "
                                         "it as a QRegularExpression programmatically, ensure the translated text has "
                                         "` {` immediately after the '%1', and '}' as the very last character, so that the "
                                         "right portion can be extracted if the user clicks on this item when it is shown "
                                         "in the QComboBox it is put in.")
                                              .arg(itSymbolUsed.key())
                                              .arg(QString::number(itSymbolUsed.value())));
            }
        }
    }
    return displayStrings;
}

void dlgRoomSymbol::accept()
{
    QDialog::accept();
    emit signal_save_symbol(getNewSymbol(), selectedColor, mpRooms);
}

QString dlgRoomSymbol::getNewSymbol()
{
    if (mpSymbols.size() <= 1) {
        return lineEdit_roomSymbol->text();
    } else {
        QRegularExpression countStripper(QStringLiteral("^(.*) {.*}$"));
        QRegularExpressionMatch match = countStripper.match(comboBox_roomSymbol->currentText());
        if (match.hasMatch() && match.lastCapturedIndex() > 0) {
            return match.captured(1);
        }
        return comboBox_roomSymbol->currentText();
    }
}

void dlgRoomSymbol::updatePreview()
{
    auto realColor = selectedColor != nullptr ? selectedColor : defaultColor();
    auto newSymbol = getNewSymbol();
    label_preview->setFont(getFontForPreview(newSymbol));
    label_preview->setText(newSymbol);
    auto bgStyle = QStringLiteral("background-color: %1; color: %2; border: 1px solid; border-radius: 1px;").arg(realColor.name(), backgroundBasedColor(realColor).name());
    pushButton_roomSymbolColor->setStyleSheet(bgStyle);
    label_preview->setStyleSheet(
            QStringLiteral("color: %1; background-color: %2; border: %3;")
                    .arg(realColor.name(), roomColor.name(), mpHost->mMapperShowRoomBorders ? QStringLiteral("1px solid %1").arg(mpHost->mRoomBorderColor.name()) : QStringLiteral("none")));
}

QFont dlgRoomSymbol::getFontForPreview(QString text) {

    auto font = mpHost->mpMap->mMapSymbolFont;
    font.setPointSize(font.pointSize() * 0.9);
    QString symbolString = text;
    QFontMetrics mapSymbolFontMetrics = QFontMetrics(font);
    QVector<quint32> codePoints = symbolString.toUcs4();
    QVector<bool> isUsable;
    for (int i = 0; i < codePoints.size(); ++i) {
        isUsable.append(mapSymbolFontMetrics.inFontUcs4(codePoints.at(i)));
    }
    bool needToFallback = isUsable.contains(false);
    if (needToFallback) {
        symbolString = QString(QChar::ReplacementCharacter);
        font.setStyleStrategy(static_cast<QFont::StyleStrategy>(mpHost->mpMap->mMapSymbolFont.styleStrategy() & ~(QFont::NoFontMerging)));
    }
    return font;
}

void dlgRoomSymbol::openColorSelector()
{
    auto* dialog = selectedColor != nullptr ? new QColorDialog(selectedColor, this) : new QColorDialog(defaultColor(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Pick color"));
    dialog->open(this, SLOT(colorSelected(const QColor&)));
    connect(dialog, &QColorDialog::currentColorChanged, this, &dlgRoomSymbol::currentColorChanged);
    connect(dialog, &QColorDialog::rejected, this, &dlgRoomSymbol::colorRejected);
}

void dlgRoomSymbol::currentColorChanged(const QColor& color)
{
    previewColor = color;
    updatePreview();
}

void dlgRoomSymbol::colorSelected(const QColor& color)
{
    selectedColor = color;
    updatePreview();
}

void dlgRoomSymbol::colorRejected()
{
    previewColor = selectedColor;
    updatePreview();
}

void dlgRoomSymbol::resetColor()
{
    selectedColor = nullptr;
    previewColor = nullptr;
    updatePreview();
}

QColor dlgRoomSymbol::backgroundBasedColor(QColor background)
{
    return background.lightness() > 127 ? Qt::black : Qt::white;
}

QColor dlgRoomSymbol::defaultColor()
{
    return backgroundBasedColor(roomColor);
}
