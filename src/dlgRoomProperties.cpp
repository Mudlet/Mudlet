/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022-2023 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2022 by Lecker Kebap - Leris@mudlet.org                 *
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
#include "TMap.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QMenu>
#include <QPainter>
#include "post_guard.h"


dlgRoomProperties::dlgRoomProperties(Host* pHost, QWidget* pParentWidget)
: QDialog(pParentWidget)
, mpHost(pHost)
{
    // init generated dialog
    setupUi(this);

    connect(lineEdit_roomSymbol, &QLineEdit::textChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(comboBox_roomSymbol, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgRoomProperties::slot_symbolComboBoxItemChanged);
    connect(comboBox_weight, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgRoomProperties::slot_weightComboBoxItemChanged);
    connect(comboBox_roomSymbol, &QComboBox::currentTextChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(pushButton_setSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openSymbolColorSelector);
    connect(pushButton_resetSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetSymbolColor);
    connect(pushButton_setRoomColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openRoomColorSelector);

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgRoomProperties::init(
    QHash<QString, int> usedNames,
    QHash<int, int>& pColors,
    QHash<QString, int>& pSymbols,
    QHash<int, int>& pWeights,
    QHash<bool, int> lockStatus,
    QSet<TRoom*>& pRooms)
{
    // Configure name display
    if (usedNames.size() > 1) {
        lineEdit_name->setText(multipleValuesPlaceholder);
        lineEdit_name->selectAll();
    } else if (usedNames.size() == 1) {
        lineEdit_name->setText(usedNames.constBegin().key());
        lineEdit_name->selectAll();
    } else {
        lineEdit_name->clear();
    }


    // Configure symbols display
    mpSymbols = pSymbols;
    mpRooms = pRooms;
    if (mpSymbols.isEmpty()) {
        // show simple text-entry box empty
        lineEdit_roomSymbol->setText(QString());
        comboBox_roomSymbol->hide();
    } else if (mpSymbols.size() == 1) {
        // show simple text-entry box with the (single) existing symbol pre-filled
        lineEdit_roomSymbol->setText(mpSymbols.constBegin().key());
        // and selected
        lineEdit_roomSymbol->selectAll();
        comboBox_roomSymbol->hide();
    } else {
        // show combined dropdown & text-entry box to host all of the (multiple) existing symbols
        lineEdit_roomSymbol->hide();
        comboBox_roomSymbol->addItems(getComboBoxSymbolItems());
        if (comboBox_roomSymbol->lineEdit()) {
            comboBox_roomSymbol->lineEdit()->selectAll();
        }
    }
    initSymbolInstructions();

    // Configure icon display
    auto pFirstRoom = *(pRooms.begin());
    selectedSymbolColor = pFirstRoom->mSymbolColor;
    if (pColors.size() == 1) {
        mRoomColor = mpHost->mpMap->getColor(pFirstRoom->getId());
        mRoomColorNumber = pColors.constBegin().key();
    } else {
        mRoomColor = QColor("grey"); // rgb(128, 128, 128)
    }
    slot_updatePreview();

    //   Configure weight display
    mpWeights = pWeights;
    if (mpWeights.isEmpty()) {
        // show spin-box with default value
        spinBox_weight->setValue(1);
        spinBox_weight->selectAll();
        comboBox_weight->hide();
    } else if (mpWeights.size() == 1) {
        // show spin-box with the (single) existing weight pre-filled
        spinBox_weight->setValue(mpWeights.constBegin().key());
        spinBox_weight->selectAll();
        comboBox_weight->hide();
    } else {
        // show combined dropdown & text-entry box to host all of the (multiple) existing weights
        spinBox_weight->hide();
        comboBox_weight->addItems(getComboBoxWeightItems());
        if (comboBox_weight->lineEdit()) {
            comboBox_weight->lineEdit()->selectAll();
        }
    }
    initWeightInstructions();

    // Configure lock display
    // Are all locks the same or mixed status? Then show dialog in tristate.
    if (lockStatus.contains(true) && lockStatus.contains(false)) {
        checkBox_locked->setTristate(true);
        checkBox_locked->setCheckState(Qt::PartiallyChecked);
    } else if (lockStatus.contains(true)) {
        checkBox_locked->setTristate(false);
        checkBox_locked->setCheckState(Qt::Checked);
    } else { // lockStatus.contains(false)
        checkBox_locked->setTristate(false);
        checkBox_locked->setCheckState(Qt::Unchecked);
    }
    initLockInstructions();

    // Configure dialog display
    adjustSize();
}


void dlgRoomProperties::initLockInstructions()
{
    const QString instructions = tr("Lock room(s), so it/they will never be used for speedwalking",
                           // Intentional comment to separate arguments!
                           "This text will be shown at a checkbox, where you can set/unset a number of room's lock.",
                           mpRooms.size());
    checkBox_locked->setText(instructions);
}


void dlgRoomProperties::initWeightInstructions()
{
    if (mpWeights.empty()) {
        label_weightInstructions->hide();
        return;
    }

    QString instructions;
    if (mpWeights.size() == 1) {
        instructions = tr("Enter a new room weight to use as the travel time for all of the %n selected room(s). "
                          "This will be used for calculating the best path. The minimum and default is 1.",
                          // Intentional comment to separate arguments!
                          "%n is the total number of rooms involved.",
                          mpRooms.size());
    } else {
        instructions = tr("To change the room weight for all of the %n selected room(s), please choose:\n"
                          " • an existing room weight from the list below (sorted by most commonly used first)\n"
                          " • enter a new positive integer value to use as a new weight. The default is 1.",
                          // Intentional comment to separate arguments!
                          "This is for when applying a new room weight to one or more rooms "
                          "and some have different weights at present. "
                          "%n is the total number of rooms involved.", mpRooms.size());
    }
    label_weightInstructions->setText(instructions);
    label_weightInstructions->setWordWrap(true);
}


void dlgRoomProperties::initSymbolInstructions()
{
    if (mpSymbols.empty()) {
        label_symbolInstructions->hide();
        return;
    }

    QString instructions;
    if (mpSymbols.size() == 1) {
        instructions = tr("Type one or more graphemes (\"visible characters\") to use as a symbol "
                          "for all of the %n selected room(s), or enter a space to clear the symbol:",
                          // Intentional comment to separate arguments!
                          "%n is the total number of rooms involved.",
                          mpRooms.size());
    } else {
        instructions = tr("To change the symbol for all of the %n selected room(s), please choose:\n"
                          " • an existing symbol from the list below (sorted by most commonly used first)\n"
                          " • enter one or more graphemes (\"visible characters\") as a new symbol\n"
                          " • enter a space to clear any existing symbols",
                          // Intentional comment to separate arguments!
                          "This is for when applying a new room symbol to one or more rooms "
                          "and some have different symbols or no symbol at present. "
                          "%n is the total number of rooms involved.", mpRooms.size());
    }
    label_symbolInstructions->setText(instructions);
    label_symbolInstructions->setWordWrap(true);
}


QStringList dlgRoomProperties::getComboBoxSymbolItems()
{
    // Obtain a set of "used" values
    QHashIterator<QString, int> itSymbolUsed(mpSymbols);
    QSet<int> symbolCountsSet;
    while (itSymbolUsed.hasNext()) {
        itSymbolUsed.next();
        symbolCountsSet.insert(itSymbolUsed.value());
    }

    // Obtains a list of those values sorted in ascending count of use
    QList<int> symbolCountsList{symbolCountsSet.begin(), symbolCountsSet.end()};
    if (symbolCountsList.size() > 1) {
        std::sort(symbolCountsList.begin(), symbolCountsList.end());
    }

    // Build a list of strings for display in descending count of use
    QStringList displayStrings;
    displayStrings.append(multipleValuesPlaceholder);
    for (int i = symbolCountsList.size() - 1; i >= 0; --i) {
        itSymbolUsed.toFront();
        while (itSymbolUsed.hasNext()) {
            itSymbolUsed.next();
            if (itSymbolUsed.value() == symbolCountsList.at(i)) {
                displayStrings.append(qsl("%1 {%2:%3}")
                    .arg(itSymbolUsed.key())
                    /*:
                    This text will be part of a list of room values shown, which will show the value
                    itself, followed by the counted number of rooms with this very value like:
                    grey {count:2} - so please translate like counted ammount, number of, etc.
                    */
                    .arg(tr("count"))
                    .arg(QString::number(itSymbolUsed.value())));
            }
        }
    }
    return displayStrings;
}


QStringList dlgRoomProperties::getComboBoxWeightItems()
{
    // Obtain a set of "used" values
    QHashIterator<int, int> itWeightUsed(mpWeights);
    QSet<int> weightCountsSet;
    while (itWeightUsed.hasNext()) {
        itWeightUsed.next();
        weightCountsSet.insert(itWeightUsed.value());
    }

    // Obtains a list of those values sorted in ascending count of use
    QList<int> weightCountsList{weightCountsSet.begin(), weightCountsSet.end()};
    if (weightCountsList.size() > 1) {
        std::sort(weightCountsList.begin(), weightCountsList.end());
    }

    // Build a list of strings for display in descending count of use
    QStringList displayStrings;
    displayStrings.append(multipleValuesPlaceholder);
    for (int i = weightCountsList.size() - 1; i >= 0; --i) {
        itWeightUsed.toFront();
        while (itWeightUsed.hasNext()) {
            itWeightUsed.next();
            if (itWeightUsed.value() == weightCountsList.at(i)) {
                displayStrings.append(qsl("%1 {%2:%3}")
                    .arg(QString::number(itWeightUsed.key()))
                    /*:
                    This text will be part of a list of room values shown, which will name the value
                    itself, followed by the counted number of rooms with that very value like:
                    grey {count: 2} - So please translate like counted amount, number of, etc.
                    */
                    .arg(tr("count"))
                    .arg(QString::number(itWeightUsed.value())));
            }
        }
    }
    return displayStrings;
}


void dlgRoomProperties::accept()
{
    QDialog::accept();

    // Find name to return back
    QString newName = lineEdit_name->text();
    bool changeName = true;
    if (newName == multipleValuesPlaceholder) {
        // We don't want to change then
        newName = QString();
        changeName = false;
    }

    // Find color to return back
    //   This is using mChangeRoomColor and mRoomColorNumber which are updated
    //   immediately when the player changes colors in the dialog (as opposed
    //   to the other room data here) - They need no further review at this time.

    // Find symbol to return back
    const QString newSymbol = getNewSymbol();
    bool changeSymbol = true;
    QColor const newSymbolColor = selectedSymbolColor;
    bool changeSymbolColor = true;
    if (newSymbol == multipleValuesPlaceholder) {
        // We don't want to change then
        changeSymbol = false;
        changeSymbolColor = false;
    }

    // Find weight to return back
    const int newWeight = getNewWeight();
    bool changeWeight = true;
    if (newWeight <= -1) {
        // We don't want to change then
        changeWeight = false;
    }

    // Find lock status to return back
    Qt::CheckState const newCheckState = checkBox_locked->checkState();
    bool changeLockStatus = true;
    bool newLockStatus;
    if (newCheckState == Qt::PartiallyChecked) {
        // We don't want to change then
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
        mChangeRoomColor, mRoomColorNumber,
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
    }
    QString newSymbolText = comboBox_roomSymbol->currentText();
    // Parse the initial text before the curly braces containing count
    QRegularExpression const countStripper(qsl("^(.*) {.*}$"));
    QRegularExpressionMatch const match = countStripper.match(newSymbolText);
    if (match.hasMatch() && match.lastCapturedIndex() > 0) {
        return match.captured(1);
    }
    return newSymbolText;
}


int dlgRoomProperties::getNewWeight()
{
    if (mpWeights.size() <= 1) {
        return spinBox_weight->value();
    }
    const QString newWeightText = comboBox_weight->currentText();
    if (newWeightText == multipleValuesPlaceholder) {
        return -1; // User did not want to select any weight, so we will do no change
    }
    // Parse an initial number out of what was selected or typed
    QRegularExpression const countStripper(qsl("^\\s*(\\d+)"));
    QRegularExpressionMatch const match = countStripper.match(newWeightText);
    if (match.hasMatch() && match.lastCapturedIndex() > 0) {
        return match.captured(1).toInt();
    }
    if (newWeightText.toInt() > 0) {
        return newWeightText.toInt();
    }
    return -2; // Maybe some other input we did not understand, so we will do no change
}


void dlgRoomProperties::slot_updatePreview()
{
    auto realSymbolColor = selectedSymbolColor.isValid() ? selectedSymbolColor : defaultSymbolColor();
    QString newSymbol = getNewSymbol();
    if (newSymbol == multipleValuesPlaceholder) {
        newSymbol = QString();
        pushButton_setSymbolColor->setStyleSheet(QString());
    } else {
        pushButton_setSymbolColor->setStyleSheet(
        qsl("background-color: %1; color: %2; border: 1px solid; border-radius: 1px;")
            .arg(realSymbolColor.name(), backgroundBasedColor(realSymbolColor).name()));
    }
    label_preview->setFont(getFontForPreview(newSymbol));
    label_preview->setText(newSymbol);
    label_preview->setStyleSheet(
        qsl("color: %1; background-color: %2; border: %3;")
            .arg(realSymbolColor.name(), mRoomColor.name(), mpHost->mMapperShowRoomBorders ? qsl("1px solid %1").arg(mpHost->mRoomBorderColor.name()) : qsl("none")));

}


QFont dlgRoomProperties::getFontForPreview(QString symbolString)
{
    auto font = mpHost->mpMap->mMapSymbolFont;
    font.setPointSize(font.pointSize() * 0.9);
    if (!symbolString.isEmpty()) {
        QFontMetrics const mapSymbolFontMetrics = QFontMetrics(font);
        QVector<quint32> const codePoints = symbolString.toUcs4();
        QVector<bool> isUsable;
        for (int i = 0; i < codePoints.size(); ++i) {
            isUsable.append(mapSymbolFontMetrics.inFontUcs4(codePoints.at(i)));
        }
        const bool needToFallback = isUsable.contains(false);
        if (needToFallback) {
            symbolString = QString(QChar::ReplacementCharacter);
            font.setStyleStrategy(static_cast<QFont::StyleStrategy>(mpHost->mpMap->mMapSymbolFont.styleStrategy() & ~(QFont::NoFontMerging)));
        }
    }
    return font;
}


void dlgRoomProperties::slot_openSymbolColorSelector()
{
    auto* dialog = new QColorDialog(selectedSymbolColor.isValid() ? selectedSymbolColor : defaultSymbolColor(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Set symbol color"));
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
    return backgroundBasedColor(mRoomColor);
}


void dlgRoomProperties::slot_selectRoomColor(QListWidgetItem* pI)
{
    mRoomColorNumber = pI->text().toInt();
}


void dlgRoomProperties::slot_defineNewColor()
{
    auto color = QColorDialog::getColor(mpHost->mRed, this, tr("Define new room color"));
    if (color.isValid()) {
        auto environmentId = mpHost->mpMap->mCustomEnvColors.size() + 257 + 16;
        if (mpHost->mpMap->mCustomEnvColors.contains(environmentId)) {
            // find a new environment ID to use, starting with the latest
            // 'safe' number so the new environment is last in the dialog
            do {
                environmentId++;
            } while (mpHost->mpMap->mCustomEnvColors.contains(environmentId));
        }

        mpHost->mpMap->mCustomEnvColors[environmentId] = color;
        slot_openRoomColorSelector();
    }
    repaint();
    mpHost->mpMap->setUnsaved(__func__);
}


void dlgRoomProperties::slot_openRoomColorSelector()
{
    auto dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Set room color"));
    auto vboxLayout = new QVBoxLayout;
    dialog->setLayout(vboxLayout);
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    dialog->setContentsMargins(0, 0, 0, 0);
    auto listWidget = new QListWidget(dialog);
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);

    connect(listWidget, &QListWidget::itemDoubleClicked, dialog, &QDialog::accept);
    connect(listWidget, &QListWidget::itemClicked, this, &dlgRoomProperties::slot_selectRoomColor);
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, this, [=]() {
        QMenu menu;
        //: This action deletes a color from the list of all room colors
        menu.addAction(tr("Delete room color"), this, [=]() {
            auto selectedItem = listWidget->takeItem(listWidget->currentRow());
            auto color = selectedItem->text();

            mpHost->mpMap->mCustomEnvColors.remove(color.toInt());
            repaint();
            mpHost->mpMap->setUnsaved(__func__);
        });

        menu.exec(QCursor::pos());
    });

    vboxLayout->addWidget(listWidget);
    auto pButtonBar = new QWidget(dialog);

    auto hboxLayout = new QHBoxLayout;
    pButtonBar->setLayout(hboxLayout);
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    auto pB_newColor = new QPushButton(pButtonBar);
    pB_newColor->setText(tr("Define new room color"));

    connect(pB_newColor, &QAbstractButton::clicked, dialog, &QDialog::reject);
    connect(pB_newColor, &QAbstractButton::clicked, this, &dlgRoomProperties::slot_defineNewColor);

    hboxLayout->addWidget(pB_newColor);

    auto pB_ok = new QPushButton(pButtonBar);
    //: confirm room color selection dialog
    pB_ok->setText(tr("OK"));
    hboxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    //: cancel room color selection dialog
    pB_abort->setText(tr("Cancel"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    hboxLayout->addWidget(pB_abort);
    vboxLayout->addWidget(pButtonBar);

    QMapIterator<int, QColor> it(mpHost->mpMap->mCustomEnvColors);
    while (it.hasNext()) {
        it.next();
        QColor c;
        c = it.value();
        auto pI = new QListWidgetItem(listWidget);
        QPixmap pix = QPixmap(50, 50);
        pix.fill(c);
        const QIcon mi(pix);
        pI->setIcon(mi);
        pI->setText(QString::number(it.key()));
        listWidget->addItem(pI);
    }
    listWidget->sortItems();

    if (dialog->exec() == QDialog::Accepted && mpHost->mpMap->mCustomEnvColors.contains(mRoomColorNumber)) {
        // Only proceed if OK pressed and color is valid - "Cancel" prevents change
        mChangeRoomColor = true;
        mRoomColor = mpHost->mpMap->mCustomEnvColors.value(mRoomColorNumber);
        slot_updatePreview();
    }
}

void dlgRoomProperties::slot_symbolComboBoxItemChanged(const int index)
{
    if (index < 0 || index >= comboBox_roomSymbol->count() || !comboBox_roomSymbol->lineEdit()) {
        return;
    }

    comboBox_roomSymbol->lineEdit()->selectAll();
}

void dlgRoomProperties::slot_weightComboBoxItemChanged(const int index)
{
    if (index < 0 || index >= comboBox_weight->count() || !comboBox_weight->lineEdit()) {
        return;
    }

    comboBox_weight->lineEdit()->selectAll();
}
