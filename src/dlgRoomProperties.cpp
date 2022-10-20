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
    connect(comboBox_roomSymbol, &QComboBox::currentTextChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(pushButton_setSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openSymbolColorSelector);
    connect(pushButton_resetSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetSymbolColor);

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgRoomProperties::init(
    QHash<QString, int> usedNames, 
    QHash<int, int>& pColors, 
    QHash<QString, int>& pSymbols, 
    QHash<int, int>& pWeights, 
    Qt::CheckState lockStatus, 
    QSet<TRoom*>& pRooms)
{
    // Configure name display
    if (usedNames.size() > 1) {
        lineEdit_name->setText(multipleValuesPlaceholder);
    } else {
        lineEdit_name->setText(usedNames.keys().first());
    }

    // Configure symbols display
    mpSymbols = pSymbols;
    mpRooms = pRooms;
    if (mpSymbols.isEmpty()) {
        // show simple text-entry box empty 
        lineEdit_roomSymbol->setText(QString());
        comboBox_roomSymbol->hide();
    } else if (mpSymbols.size() = 1) {
        // show simple text-entry box with the (single) existing symbol pre-filled
        lineEdit_roomSymbol->setText(mpSymbols.keys().first());
        comboBox_roomSymbol->hide();
    } else {
        // show combined dropdown & text-entry box to host all of the (multiple) existing symbols
        lineEdit_roomSymbol->hide();
        comboBox_roomSymbol->addItems(getComboBoxSymbolItems());
    }
    initSymbolInstructionLabel();

    // Configure icon display
    if (!pRooms.isEmpty()) {
        auto pRoom = *(pRooms.begin());
        if (pRoom) {
            auto firstRoomId = pRoom->getId();
            roomColor = mpHost->mpMap->getColor(firstRoomId);
            selectedSymbolColor = pRoom->mSymbolColor;
        }
    }
    slot_updatePreview();

    // TODO: https://github.com/Mudlet/Mudlet/pull/6354
    //   Configure weight display

    // Configure lock display
    if (lockStatus == Qt::PartiallyChecked) {
        checkBox_locked->setTristate(true);
    } else {
        checkBox_locked->setTristate(false);
    }
    checkBox_locked->setCheckState(lockStatus);

    // Configure dialog display
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
        instructions = tr("To change the symbol for all of the %n selected room(s), please choose:\n"
                          " • an existing symbol from the list below (sorted by most commonly used first)\n"
                          " • enter one or more graphemes (\"visible characters\") as a new symbol\n"
                          " • enter a space to clear any existing symbols",
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
    displayStrings.append(multipleValuesPlaceholder);
    for (int i = symbolCountsList.size() - 1; i >= 0; --i) {
        itSymbolUsed.toFront();
        while (itSymbolUsed.hasNext()) {
            itSymbolUsed.next();
            if (itSymbolUsed.value() == symbolCountsList.at(i)) {
                displayStrings.append(tr("%1 (count:%2)",
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

    // find name to return back
    QString newName = lineEdit_name->text();
    bool changeName = true;
    if (newName == multipleValuesPlaceholder) {
        // We don't want to change the name then
        newName = QString();
        changeName = false;
    }

    // TODO: https://github.com/Mudlet/Mudlet/pull/6354
    //   find color (if any) to return back
    int newRoomColor = 1;
    bool changeRoomColor = false;

    // find symbol to return back
    QString newSymbol = getNewSymbol();
    bool changeSymbol = true;
    QColor newSymbolColor = selectedSymbolColor;
    bool changeSymbolColor = true;
    if (newSymbol == multipleValuesPlaceholder) {
        // We don't want to change the symbol then
        changeSymbol = false;
        changeSymbolColor = false;
    }

    // TODO: https://github.com/Mudlet/Mudlet/pull/6354
    //   find weight (if any) to return back
    int newWeight = 1;
    bool changeWeight = false;

    // find lock status to return back
    Qt::CheckState newCheckState = checkBox_locked->checkState();
    bool changeLockStatus = true;
    bool newLockStatus;
    if (newCheckState == Qt::PartiallyChecked) {
        // We don't want to change the lock then
        changeLockStatus = false;
    } else {
        if (newCheckState == Qt::Checked) {
            newLockStatus = true;
        } else { // Qt::Unchecked
            newLockStatus = false;
        }
    }

    emit signal_save_symbol(
        changeName, newName,
        changeRoomColor, newRoomColor,
        changeSymbol, newSymbol,
        changeSymbolColor, newSymbolColor,
        changeWeight, newWeight,
        changeLockStatus, newLockStatus,
        mpRooms);
}

QString dlgRoomProperties::getNewSymbol()
{
    if (mpSymbols.size() <= 1) {
        return lineEdit_roomSymbol->text();
    } else {
    // TODO: https://github.com/Mudlet/Mudlet/pull/6354
    //   Add knowledge from https://github.com/Mudlet/Mudlet/pull/6359 to here as well
    //   Maybe refactor both sections to use same regex logic to prevent this situation?
        QRegularExpression countStripper(qsl("^(.*) \\(.*\\)$"));
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
    QString newSymbol = getNewSymbol();
    if (newSymbol == multipleValuesPlaceholder) {
        newSymbol = QString();
    // TODO: https://github.com/Mudlet/Mudlet/pull/6354
    //   Add placeholder icon (color?) to here as well, for example light grey rectangle
    }
    label_preview->setFont(getFontForPreview(newSymbol));
    label_preview->setText(newSymbol);
    label_preview->setStyleSheet(
        qsl("color: %1; background-color: %2; border: %3;")
            .arg(realSymbolColor.name(), roomColor.name(), mpHost->mMapperShowRoomBorders ? qsl("1px solid %1").arg(mpHost->mRoomBorderColor.name()) : qsl("none")));
    pushButton_setSymbolColor->setStyleSheet(
        qsl("background-color: %1; color: %2; border: 1px solid; border-radius: 1px;")
            .arg(realSymbolColor.name(), backgroundBasedColor(realSymbolColor).name()));
}

QFont dlgRoomProperties::getFontForPreview(QString symbolString)
{
    auto font = mpHost->mpMap->mMapSymbolFont;
    font.setPointSize(font.pointSize() * 0.9);
    if (!symbolString.isEmpty()) {
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
    }
    return font;
}

void dlgRoomProperties::slot_openSymbolColorSelector()
{
    auto* dialog = (selectedSymbolColor != nullptr) ? new QColorDialog(selectedSymbolColor, this) : new QColorDialog(defaultSymbolColor(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Pick color"));
    dialog->open(this, SLOT(slot_symbolColorSelected(const QColor&)));
}

void dlgRoomProperties::slot_symbolColorSelected(const QColor& color)
{
    selectedSymbolColor = color;
    slot_updatePreview();
}

void dlgRoomProperties::slot_resetSymbolColor()
{
    selectedSymbolColor = QColor();
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
