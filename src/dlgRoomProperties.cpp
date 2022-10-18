/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
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


#include "dlgRoomProperties.h"
#include "Host.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QPainter>
#include "post_guard.h"

dlgRoomProperties::dlgRoomProperties(Host* pHost, QWidget* pParentWidget)
: QDialog(pParentWidget)
, mpHost(pHost)
{
    // init generated dialog
    setupUi(this);

    connect(lineEdit_roomSymbol, &QLineEdit::textChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(pushButton_setSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openSymbolColorSelector);
    connect(pushButton_resetSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetSymbolColor);
    connect(comboBox_roomSymbol, &QComboBox::currentTextChanged, this, &dlgRoomProperties::slot_updatePreview);

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgRoomProperties::init(QHash<QString, int> usedNames, QHash<int, int>& pColors, QHash<QString, int>& pSymbols, QHash<int, int>& pWeights, int lockStatus, QSet<TRoom*>& pRooms)
{
    // Configure display in preview section
    if (usedNames.size() == 1) {
        lineEdit_name->setPlaceholderText(tr("Multiple values..."));
    } else {
        lineEdit_name->setText(usedNames.keys().first());
    }

    // Configure display in symbol section
    mpSymbols = pSymbols;
    mpRooms = pRooms;
    if (mpSymbols.size() <= 1) {
        // show simple text-entry box, either empty or with the (single) existing symbol pre-filled
        lineEdit_roomSymbol->setText(!mpSymbols.isEmpty() ? mpSymbols.keys().first() : QString());
        comboBox_roomSymbol->hide();
    } else {
        // show combined dropdown & text-entry box to host all of the (multiple) existing symbols
        lineEdit_roomSymbol->hide();
        comboBox_roomSymbol->addItems(getComboBoxSymbolItems());
    }
    initSymbolInstructionLabel();
    if (!pRooms.isEmpty()) {
        auto pRoom = *(pRooms.begin());
        if (pRoom) {
            auto firstRoomId = pRoom->getId();
            selectedSymbolColor = pRoom->mSymbolColor;
            previewSymbolColor = pRoom->mSymbolColor;
            roomColor = mpHost->mpMap->getColor(firstRoomId);
        }
    }
    slot_updatePreview();

    // Configure display in pathfinding section
    if (lockStatus == Qt::PartiallyChecked) {
        checkBox_locked->setTristate(true);
    }
    checkBox_locked->setCheckState(lockStatus);

    adjustSize();
}

void dlgRoomProperties::initSymbolInstructionLabel()
{
    if (mpSymbols.empty()) {
        label_symbolInstructions->hide();
        return;
    }

    QString instructions;
    if (mpSymbols.size() == 1) {
        if (mpRooms.size() > 1) {
            instructions = tr("The only used symbol is \"%1\" in one or "
                              "more of the selected %n room(s), delete this to "
                              "clear it from all selected rooms or replace "
                              "with a new symbol to use for all the rooms:",
                              // Intentional comment to separate arguments!
                              "This is for when applying a new room symbol to one or more rooms "
                              "and some have the SAME symbol (others may have none) at present, "
                              "%n is the total number of rooms involved and is at least two. ",
                              mpRooms.size()).arg(mpSymbols.keys().first());
        } else {
            instructions = tr("The symbol is \"%1\" in the selected room, "
                              "delete this to clear the symbol or replace "
                              "it with a new symbol for this room:",
                              // Intentional comment to separate arguments!
                              "This is for when applying a new room symbol to one room. ")
                                   .arg(mpSymbols.keys().first());
        }
    } else {
        instructions = tr("Choose:\n"
                          " • an existing symbol from the list below (sorted by most commonly used first)\n"
                          " • enter one or more graphemes (\"visible characters\") as a new symbol\n"
                          " • enter a space to clear any existing symbols\n"
                          "for all of the %n selected room(s):",
                          // Intentional comment to separate arguments!
                          "This is for when applying a new room symbol to one or more rooms "
                          "and some have different symbols (others may have none) at present, "
                          "%n is the number of rooms involved.", mpRooms.size());
    }
    label_symbolInstructions->setText(instructions);
    label_symbolInstructions->setWordWrap(true);
}

QStringList dlgRoomProperties::getComboBoxSymbolItems()
{
    QHashIterator<QString, int> itSymbolUsed(mpSymbols);
    QSet<int> symbolCountsSet;
    while (itSymbolUsed.hasNext()) {
        itSymbolUsed.next();
        symbolCountsSet.insert(itSymbolUsed.value());
    }
    QList<int> symbolCountsList{symbolCountsSet.begin(), symbolCountsSet.end()};
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

void dlgRoomProperties::accept()
{
    QDialog::accept();

    QString newName = lineEdit_name->text();
    int newRoomColor = 1; // TODO FIXME
    int newWeight = 5; // TODO FIXME
    Qt::CheckState newLockStatus = checkBox_locked->checkState();

    emit signal_save_symbol(newName, newRoomColor, getNewSymbol(), selectedSymbolColor, newWeight, newLockStatus, mpRooms);
}

QString dlgRoomProperties::getNewSymbol()
{
    if (mpSymbols.size() <= 1) {
        return lineEdit_roomSymbol->text();
    } else {
        QRegularExpression countStripper(qsl("^(.*) {.*}$"));
        QRegularExpressionMatch match = countStripper.match(comboBox_roomSymbol->currentText());
        if (match.hasMatch() && match.lastCapturedIndex() > 0) {
            return match.captured(1);
        }
        return comboBox_roomSymbol->currentText();
    }
}

void dlgRoomProperties::slot_updatePreview()
{
    auto realSymbolColor = selectedSymbolColor != nullptr ? selectedSymbolColor : defaultSymbolColor();
    auto newSymbol = getNewSymbol();
    label_preview->setFont(getFontForPreview(newSymbol));
    label_preview->setText(newSymbol);
    label_preview->setStyleSheet(
        qsl("color: %1; background-color: %2; border: %3;")
            .arg(realSymbolColor.name(), roomColor.name(), mpHost->mMapperShowRoomBorders ? qsl("1px solid %1").arg(mpHost->mRoomBorderColor.name()) : qsl("none")));
    pushButton_setSymbolColor->setStyleSheet(
        qsl("background-color: %1; color: %2; border: 1px solid; border-radius: 1px;")
            .arg(realSymbolColor.name(), backgroundBasedColor(realSymbolColor).name()));
}

QFont dlgRoomProperties::getFontForPreview(QString text)
{
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

void dlgRoomProperties::slot_openSymbolColorSelector()
{
    auto* dialog = selectedSymbolColor != nullptr ? new QColorDialog(selectedSymbolColor, this) : new QColorDialog(defaultSymbolColor(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Pick color"));
    dialog->open(this, SLOT(slot_symbolColorSelected(const QColor&)));
    connect(dialog, &QColorDialog::currentColorChanged, this, &dlgRoomProperties::slot_currentSymbolColorChanged);
    connect(dialog, &QColorDialog::rejected, this, &dlgRoomProperties::slot_symbolColorRejected);
}

void dlgRoomProperties::slot_currentSymbolColorChanged(const QColor& color)
{
    previewSymbolColor = color;
    slot_updatePreview();
}

void dlgRoomProperties::slot_symbolColorSelected(const QColor& color)
{
    selectedSymbolColor = color;
    slot_updatePreview();
}

void dlgRoomProperties::slot_symbolColorRejected()
{
    previewSymbolColor = selectedSymbolColor;
    slot_updatePreview();
}

void dlgRoomProperties::slot_resetSymbolColor()
{
    selectedSymbolColor = QColor();
    previewSymbolColor = QColor();
    slot_updatePreview();
}

QColor dlgRoomProperties::backgroundBasedColor(QColor background)
{
    return background.lightness() > 127 ? Qt::black : Qt::white;
}

QColor dlgRoomProperties::defaultSymbolColor()
{
    return backgroundBasedColor(roomColor);
}
