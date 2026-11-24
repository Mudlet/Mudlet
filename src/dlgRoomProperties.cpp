/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022-2023, 2025-2026 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2022-2025 by Lecker Kebap - Leris@mudlet.org            *
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
#include "AlphaColourWell.h"
#include "Host.h"
#include "RoomAppearance.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"

#include <QColorDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include <limits>

namespace {
// The map draws from these keys itself, and the dialog edits the border ones elsewhere, so the table neither shows
// nor writes them
const QLatin1String scmMudletOwnedUserDataPrefix{"room.ui_"};

// The key the generic mapper keeps a room's description under; the General tab edits it, so the table does not
const QLatin1String scmDescriptionKey{"description"};

bool editedInUserDataTable(const QString& key)
{
    return !key.startsWith(scmMudletOwnedUserDataPrefix) && key != scmDescriptionKey;
}

// What toPlainText() gives, but with non-breaking spaces kept: it turns them into plain ones, and a mapper script
// matching the game's description would then miss the room
QString plainTextKeepingSpaces(const QPlainTextEdit& edit)
{
    QString text = edit.document()->toRawText();
    for (QChar& character : text) {
        switch (character.unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            character = QLatin1Char('\n');
            break;
        default:
            break;
        }
    }
    return text;
}

// Set on a value cell that shows the placeholder because the selected rooms do not agree on that key
constexpr int scmMixedValueRole = Qt::UserRole;
// Set for good on a value cell that started out mixed: emptied, even after a value was typed in, it goes back to the
// placeholder rather than writing nothing to every room
constexpr int scmStartedMixedRole = Qt::UserRole + 1;

// Opens a cell showing the placeholder with an empty editor, and leaves the cell undecided when nothing was typed or
// it was emptied again, so that only a new value replaces the rooms' differing ones, as on the description field
class UserDataValueDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
        // A line edit holds 32767 characters by default and cuts a longer value it is opened on
        if (auto* lineEdit = qobject_cast<QLineEdit*>(editor)) {
            lineEdit->setMaxLength(std::numeric_limits<int>::max());
        }
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (lineEdit && index.data(scmMixedValueRole).toBool()) {
            lineEdit->clear();
            return;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* lineEdit = qobject_cast<QLineEdit*>(editor);
        // Nothing typed, nothing to write back
        if (lineEdit && !lineEdit->isModified()) {
            return;
        }
        if (lineEdit && index.data(scmMixedValueRole).toBool() && lineEdit->text().isEmpty()) {
            return;
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }
};
} // namespace


dlgRoomProperties::dlgRoomProperties(Host* pHost, QWidget* pParentWidget)
: QDialog(pParentWidget)
, mpHost(pHost)
{
    // init generated dialog
    setupUi(this);
    mBorderColorButtonWords = pushButton_setBorderColor->text();
    // The well's words say how opaque the colour is, so its name says what it sets
    pushButton_setBorderColor->setAccessibleName(mBorderColorButtonWords);

    connect(lineEdit_roomSymbol, &QLineEdit::textChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(comboBox_roomSymbol, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgRoomProperties::slot_symbolComboBoxItemChanged);
    connect(comboBox_weight, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgRoomProperties::slot_weightComboBoxItemChanged);
    connect(comboBox_roomSymbol, &QComboBox::currentTextChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(pushButton_setSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openSymbolColorSelector);
    connect(pushButton_resetSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetSymbolColor);
    connect(pushButton_setRoomColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openRoomColorSelector);
    connect(pushButton_setBorderColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openBorderColorSelector);
    connect(pushButton_resetBorderColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetBorderColor);
    connect(spinBox_borderThickness, qOverload<int>(&QSpinBox::valueChanged), this, &dlgRoomProperties::slot_borderThicknessChanged);
    connect(pushButton_addUserData, &QAbstractButton::clicked, this, &dlgRoomProperties::slot_addUserData);
    connect(pushButton_removeUserData, &QAbstractButton::clicked, this, &dlgRoomProperties::slot_removeUserData);
    connect(tableWidget_userData, &QTableWidget::itemChanged, this, &dlgRoomProperties::slot_userDataItemChanged);
    tableWidget_userData->setItemDelegateForColumn(1, new UserDataValueDelegate(tableWidget_userData));
    connect(lineEdit_hash, &QLineEdit::textChanged, this, &dlgRoomProperties::slot_hashChanged);
    connect(plainTextEdit_description, &QPlainTextEdit::textChanged, this, [this]() {
        mDescriptionWasEdited = true;
    });

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgRoomProperties::init(
        QHash<QString, int> usedNames, QHash<int, int>& pColors, QHash<QString, int>& pSymbols, QHash<int, int>& pWeights, QHash<bool, int> lockStatus, int hiddenRoomCount, const QSet<int>& roomIds)
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
    mRoomIds = roomIds;

    // Store original border values for live preview restoration on cancel
    TRoom* pFirstRoom = nullptr;
    for (const int roomId : std::as_const(mRoomIds)) {
        TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomId);
        if (!room) {
            continue;
        }
        if (!pFirstRoom) {
            pFirstRoom = room;
        }
        mRoomSerials[roomId] = room->serial();
        mOriginalBorderColors[roomId] = room->mBorderColor;
        mOriginalBorderThicknesses[roomId] = room->mBorderThickness;
    }

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
    auto mapSymbolFont = mpHost->mpMap->mMapSymbolFont;
    mapSymbolFont.setPointSize(qApp->font().pointSize());
    if (!lineEdit_roomSymbol->isHidden()) {
        lineEdit_roomSymbol->setFont(mapSymbolFont);
    }
    if (!comboBox_roomSymbol->isHidden()) {
        comboBox_roomSymbol->setFont(mapSymbolFont);
    }
    initSymbolInstructions();

    // Configure icon display
    if (pFirstRoom) {
        selectedSymbolColor = pFirstRoom->mSymbolColor;
    }
    if (pFirstRoom && pColors.size() == 1) {
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

    // Configure hidden display
    if (hiddenRoomCount && hiddenRoomCount < mRoomIds.size()) {
        // Some rooms are hidden
        checkBox_hidden->setTristate(true);
        checkBox_hidden->setCheckState(Qt::PartiallyChecked);
    } else {
        // Either none or all of them are hidden
        checkBox_hidden->setTristate(false);
        checkBox_hidden->setChecked(hiddenRoomCount == mRoomIds.size());
    }
    initHiddenInstructions(hiddenRoomCount);

    // Configure border display
    selectedBorderColor = pFirstRoom->mBorderColor;
    mBorderThickness = pFirstRoom->mBorderThickness;

    showBorderColor();

    // Set thickness spinbox
    spinBox_borderThickness->setValue(mBorderThickness);
    initBorderInstructions();

    initGeneral();
    initUserData();

    // The name is what is most often changed: typed straight away, Enter renames. Set here, as the buttons not in the
    // tab order would otherwise be the first to take focus
    lineEdit_name->setFocus(Qt::OtherFocusReason);

    // Configure dialog display
    adjustSize();
}


void dlgRoomProperties::initLockInstructions()
{
    //: room properties dialog, text will be shown at a checkbox, where you can set/unset a number of room's lock.
    const QString instructions = tr("Lock %n room(s), so it/they will never be used for speedwalking", nullptr, mRoomIds.size());
    checkBox_locked->setText(instructions);
}

void dlgRoomProperties::initHiddenInstructions(const int hiddenRoomCount)
{
    if (hiddenRoomCount && hiddenRoomCount < mRoomIds.size()) {
        checkBox_hidden->setText(
                /*: room properties dialog, setting text for checkbox, where you can
 set/unset a number of room's hidden status. More than one room is being
 considered and some, but not all (%n) of them are hidden and in this case the
 checkbox also has an partially checked state to be used to leave them all
 unchanged. A second translatable sentance indicating the number of currently
 hidden rooms will be inserted as %1.*/
                tr("Hide all %n room(s).%1", nullptr, mRoomIds.size())
                        /*: room properties dialog, additional sentance inserted into setting text for checkbox,
 when some (%n) but not all of the rooms are hidden. Ensure that, if the locale uses
 spaces between words, that one is present at the beginning or end so that the
 text is correctly spaced when it is inserted into the primary text.*/
                        .arg(tr(" %n room(s) are currently hidden.", nullptr, hiddenRoomCount)));
        /*: Tooltip to give additional information for the checkbox to control the
 state of being hidden when the selection includes multiple rooms and they
 are not all in the same state.*/
        checkBox_hidden->setToolTip(utils::richText(tr("Leave as partially checked to not change the state of the selected rooms.")));
        return;
    }

    /*: room properties dialog, setting text for checkbox, where you can
 set/unset the hidden status of one or more rooms where %n is the total number
 of rooms and all of them are currently hidden or shown.*/
    checkBox_hidden->setText(tr("Hide (all) %n room(s).", nullptr, mRoomIds.size()));
    checkBox_hidden->setToolTip(QString());
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
                          mRoomIds.size());
    } else {
        instructions = tr("To change the room weight for all of the %n selected room(s), please choose:\n"
                          " • an existing room weight from the list below (sorted by most commonly used first)\n"
                          " • enter a new positive integer value to use as a new weight. The default is 1.",
                          // Intentional comment to separate arguments!
                          "This is for when applying a new room weight to one or more rooms "
                          "and some have different weights at present. "
                          "%n is the total number of rooms involved.",
                          mRoomIds.size());
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
        //: room properties dialog, setting symbols
        instructions = tr("Enter one or more characters to set a new symbol for %n room(s).  Clear to unset.",
                          // Intentional comment to separate arguments!
                          "%n is the total number of rooms involved.",
                          mRoomIds.size());
    } else {
        //: room properties dialog, setting symbols
        instructions = tr("To set the symbol for all %n room(s), please choose:\n"
                          " • an existing symbol from the list,\n"
                          " • enter one or more characters to set a new symbol,\n"
                          " • clear to unset.",
                          // Intentional comment to separate arguments!
                          "This is for when applying a new room symbol to one or more rooms "
                          "and some have different symbols or no symbol at present. "
                          "%n is the total number of rooms involved.",
                          mRoomIds.size());
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
                /*: Format for showing a room symbol with its usage count. %1 is the symbol itself (e.g., "★" or "!"),
%2 is the number of rooms using this symbol. Example output: "★ (count: 5)" or "! (count: 12)".
The word "count" and the format can be translated, but ensure the numbers remain clearly associated.*/
                displayStrings.append(tr("%1 (count: %2)").arg(itSymbolUsed.key(), QString::number(itSymbolUsed.value())));
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
                /*: Format for showing a room weight with its usage count. %1 is the weight value (e.g., "1" or "50"),
 %2 is the number of rooms with this weight. Example output: "5 (count: 3)" or "100 (count: 7)".
 The word "count" and the format can be translated, but ensure the numbers remain clearly associated.*/
                displayStrings.append(tr("%1 (count: %2)").arg(QString::number(itWeightUsed.key()), QString::number(itWeightUsed.value())));
            }
        }
    }
    return displayStrings;
}


void dlgRoomProperties::accept()
{
    // Leaving the cell commits an edit still open in it: a button on macOS takes no focus when clicked, so the editor
    // would not have lost it, and what was typed would go unsaved
    tableWidget_userData->setCurrentIndex(QModelIndex());
    QDialog::accept();

    // Find name to return back
    QString newName = lineEdit_name->text();
    bool changeName = true;
    if (newName == multipleValuesPlaceholder) {
        // We don't want to change then
        newName = QString();
        changeName = false;
    }

    // mChangeRoomColor and mRoomColorNumber were already set when the color selector closed with OK

    // Find symbol to return back
    const QString newSymbol = getNewSymbol();
    bool changeSymbol = true;
    const QColor newSymbolColor = selectedSymbolColor;
    bool changeSymbolColor = mSymbolColorWasChanged;
    if (newSymbol == multipleValuesPlaceholder) {
        // We don't want to change then
        changeSymbol = false;
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
    std::optional<bool> newLockStatus;
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

    // Find hidden status to return back
    Qt::CheckState const newHiddenCheckState = checkBox_hidden->checkState();
    bool changeHiddenStatus = true;
    std::optional<bool> newHiddenStatus;
    if (newHiddenCheckState == Qt::PartiallyChecked) {
        // We don't want to change then
        changeHiddenStatus = false;
    } else {
        if (newHiddenCheckState == Qt::Checked) {
            newHiddenStatus = true;
        } else { // Qt::Unchecked
            newHiddenStatus = false;
        }
    }

    // Find border settings to return back
    bool changeBorderColor = mBorderColorWasChanged;
    QColor newBorderColor = selectedBorderColor;
    bool changeBorderThickness = mBorderThicknessWasChanged;
    int newBorderThickness = mBorderThickness;

    emit signal_save_symbol(changeName,
                            newName,
                            mChangeRoomColor,
                            mRoomColorNumber,
                            changeSymbol,
                            newSymbol,
                            changeSymbolColor,
                            newSymbolColor,
                            changeWeight,
                            newWeight,
                            changeLockStatus,
                            newLockStatus,
                            changeHiddenStatus,
                            newHiddenStatus,
                            changeBorderColor,
                            newBorderColor,
                            changeBorderThickness,
                            newBorderThickness,
                            liveRoomIds());

    QMap<QString, std::optional<QString>> changes = userDataChanges();
    if (mDescriptionWasEdited) {
        QString description = plainTextKeepingSpaces(*plainTextEdit_description);
        // The field reads "\r\n" as one line break and gives back "\n": a description stored with them gets them back,
        // so that a mapper script matching it as the game sent it still finds the room
        if (mOriginalDescription.value_or(QString()).contains(qsl("\r\n"))) {
            description.replace(QLatin1Char('\n'), qsl("\r\n"));
        }
        // Rooms that started out with different descriptions keep them unless one is typed in: empty again, the field is
        // back to its placeholder rather than set to nothing
        const bool mixedAndLeftEmpty = !mOriginalDescription.has_value() && description.isEmpty();
        if (!mixedAndLeftEmpty && description != mOriginalDescription) {
            changes.insert(scmDescriptionKey, description.isEmpty() ? std::nullopt : std::optional<QString>(description));
        }
    }
    if (!changes.isEmpty()) {
        emit signal_save_userData(changes, liveRoomIds());
    }

    const QSet<int> rooms = liveRoomIds();
    if (rooms.size() == 1 && lineEdit_hash->text() != mOriginalHash) {
        emit signal_save_hash(*rooms.cbegin(), lineEdit_hash->text());
    }

    // Last, as the map then shows the area the rooms went to
    const QVariant areaId = comboBox_area->currentData();
    if (areaId.isValid() && areaId.toInt() != mOriginalAreaId && !rooms.isEmpty()) {
        emit signal_save_area(areaId.toInt(), rooms);
    }
}


QString dlgRoomProperties::getNewSymbol()
{
    if (mpSymbols.size() <= 1) {
        return lineEdit_roomSymbol->text();
    }
    QString newSymbolText = comboBox_roomSymbol->currentText();
    // Parse the initial text before the parentheses containing count
    const QRegularExpression countStripper(qsl("^(.*) \\(.*\\)$"));
    const QRegularExpressionMatch match = countStripper.match(newSymbolText);
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
    const QRegularExpression countStripper(qsl("^\\s*(\\d+)"));
    const QRegularExpressionMatch match = countStripper.match(newWeightText);
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
                qsl("background-color: %1; color: %2; border: 1px solid; border-radius: 1px;").arg(realSymbolColor.name(), backgroundBasedColor(realSymbolColor).name()));
    }
    const bool roundRoom = mpHost->mBubbleMode;
    label_preview->setStyleSheet(qsl("background-color: %1; border: %2; border-radius: %3px;")
                                         .arg(mRoomColor.name(),
                                              mpHost->mMapperShowRoomBorders ? qsl("1px solid %1").arg(mpHost->mRoomBorderColor.name()) : qsl("none"),
                                              QString::number(roundRoom ? label_preview->width() / 2 : 0)));
    label_preview->setAccessibleName(newSymbol);
    if (newSymbol.isEmpty()) {
        label_preview->clear();
        return;
    }
    // Drawn as the map draws it, which shrinks a symbol to fit the room, and
    // a round room's to fit the square inside its circle
    const QSize size = label_preview->contentsRect().size();
    QRect symbolRect(QPoint(), roundRoom ? size * 0.707 : size);
    symbolRect.moveCenter(QRect(QPoint(), size).center());
    const qreal ratio = label_preview->devicePixelRatioF();
    QPixmap pixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    ushort fontSize = 1;
    RoomAppearance::paintSymbol(painter, symbolRect, newSymbol, realSymbolColor, mpHost->mpMap->mMapSymbolFont, mpHost->mpMap->mMapSymbolFontFudgeFactor, fontSize);
    painter.end();
    label_preview->setPixmap(pixmap);
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
    mSymbolColorWasChanged = true;
    slot_updatePreview();
}


void dlgRoomProperties::slot_resetSymbolColor()
{
    selectedSymbolColor = QColor();
    mSymbolColorWasChanged = true;
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

    // Held apart from mRoomColorNumber until OK, so a Cancel discards the click
    int clickedColorNumber = mRoomColorNumber;
    connect(listWidget, &QListWidget::itemDoubleClicked, dialog, &QDialog::accept);
    connect(listWidget, &QListWidget::itemClicked, dialog, [&clickedColorNumber](QListWidgetItem* pI) {
        clickedColorNumber = pI->text().toInt();
    });
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
        // A right-click on empty space leaves the current item as it was, so
        // only the color under the cursor is offered for deletion
        QListWidgetItem* clickedItem = listWidget->itemAt(pos);
        if (!clickedItem) {
            return;
        }
        QMenu menu;
        //: This action deletes a color from the list of all room colors
        menu.addAction(tr("Delete room color"), this, [=, this]() {
            const int colorNumber = clickedItem->text().toInt();
            delete clickedItem;

            mpHost->mpMap->mCustomEnvColors.remove(colorNumber);
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

    if (dialog->exec() == QDialog::Accepted && mpHost->mpMap->mCustomEnvColors.contains(clickedColorNumber)) {
        mRoomColorNumber = clickedColorNumber;
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

void dlgRoomProperties::initBorderInstructions()
{
    //: Instruction text shown in room properties dialog for the border customization section
    QString instructions = tr("Set a custom border color and thickness for the selected room(s). "
                              "Leave at default to use the global map settings.");
    label_borderInstructions->setText(instructions);
    label_borderInstructions->setWordWrap(true);
}

void dlgRoomProperties::slot_openBorderColorSelector()
{
    QColor initialColor = selectedBorderColor.isValid() ? selectedBorderColor : mpHost->mRoomBorderColor;
    auto* dialog = new QColorDialog(initialColor, this);
    dialog->setOption(QColorDialog::ShowAlphaChannel, true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    //: Title for the color picker dialog when selecting a room border color
    dialog->setWindowTitle(tr("Set border color"));
    connect(dialog, &QColorDialog::currentColorChanged, this, &dlgRoomProperties::slot_borderColorSelected);
    connect(dialog, &QColorDialog::colorSelected, this, &dlgRoomProperties::slot_borderColorSelected);
    dialog->open();
}

void dlgRoomProperties::slot_borderColorSelected(const QColor& color)
{
    selectedBorderColor = color;
    mBorderColorWasChanged = true;
    showBorderColor();
    emitBorderPreview();
}

// A border colour may be see-through, so it is shown as a swatch over a checkerboard with how opaque it is;
// with none chosen the button goes back to saying what it does
void dlgRoomProperties::showBorderColor()
{
    uiDesign::AlphaColourWell::on(pushButton_setBorderColor)->setColour(selectedBorderColor);
    if (!selectedBorderColor.isValid()) {
        pushButton_setBorderColor->setText(mBorderColorButtonWords);
    }
}

void dlgRoomProperties::slot_resetBorderColor()
{
    selectedBorderColor = QColor();
    mBorderColorWasChanged = true;
    showBorderColor();
    emitBorderPreview();
}

void dlgRoomProperties::slot_borderThicknessChanged(int value)
{
    mBorderThickness = value;
    mBorderThicknessWasChanged = true;
    emitBorderPreview();
}

void dlgRoomProperties::initGeneral()
{
    QList<const TRoom*> rooms;
    for (const int roomId : std::as_const(mRoomIds)) {
        if (const TRoom* room = liveRoom(roomId)) {
            rooms << room;
        }
    }
    const bool single = rooms.size() == 1;

    //: Shown in Configure room's General tab in place of a room ID when several rooms are being configured at once
    label_roomIdValue->setText(single ? QString::number(rooms.first()->getId()) : tr("%n room(s) selected", nullptr, rooms.size()));

    // A game ID belongs to one room only, so it cannot be given to a selection
    const QSignalBlocker hashBlocker(lineEdit_hash);
    mOriginalHash = single ? mpHost->mpMap->mpRoomDB->roomIDToHash.value(rooms.first()->getId()) : QString();
    lineEdit_hash->setText(mOriginalHash);
    lineEdit_hash->setEnabled(single);
    //: Placeholder of Configure room's game ID field, which is disabled when several rooms are being configured, as a game ID belongs to one room
    lineEdit_hash->setPlaceholderText(single ? QString() : tr("Only for a single room"));
    label_hashWarning->hide();
    label_userDataWarning->hide();

    QSet<int> roomAreas;
    for (const TRoom* room : std::as_const(rooms)) {
        roomAreas.insert(room->getArea());
    }
    mOriginalAreaId = roomAreas.size() == 1 ? std::optional<int>(*roomAreas.cbegin()) : std::nullopt;
    QList<QPair<QString, int>> areas;
    const QMap<int, QString>& areaNames = mpHost->mpMap->mpRoomDB->getAreaNamesMap();
    for (auto it = areaNames.cbegin(); it != areaNames.cend(); ++it) {
        areas.append({it.value(), it.key()});
    }
    std::sort(areas.begin(), areas.end(), [](const auto& left, const auto& right) {
        const int byName = QString::localeAwareCompare(left.first, right.first);
        return byName ? byName < 0 : left.second < right.second;
    });
    const QSignalBlocker areaBlocker(comboBox_area);
    comboBox_area->clear();
    if (!mOriginalAreaId.has_value()) {
        // No area id, so leaving it picked moves no room
        comboBox_area->addItem(multipleValuesPlaceholder);
    }
    for (const auto& [name, id] : std::as_const(areas)) {
        comboBox_area->addItem(qsl("%1 (%2)").arg(name, QString::number(id)), id);
    }
    comboBox_area->setCurrentIndex(mOriginalAreaId.has_value() ? comboBox_area->findData(*mOriginalAreaId) : 0);

    QSet<QString> descriptions;
    bool someWithout = false;
    for (const TRoom* room : std::as_const(rooms)) {
        const auto it = room->userData.constFind(scmDescriptionKey);
        if (it == room->userData.cend()) {
            someWithout = true;
        } else {
            descriptions.insert(it.value());
        }
    }
    if (descriptions.isEmpty()) {
        mOriginalDescription = QString();
    } else if (descriptions.size() == 1 && !someWithout) {
        mOriginalDescription = *descriptions.cbegin();
    } else {
        mOriginalDescription = std::nullopt;
    }
    const QSignalBlocker descriptionBlocker(plainTextEdit_description);
    plainTextEdit_description->setPlainText(mOriginalDescription.value_or(QString()));
    //: Placeholder of Configure room's description field while it is empty
    plainTextEdit_description->setPlaceholderText(mOriginalDescription.has_value() ? tr("Room description...") : multipleValuesPlaceholder);
    mDescriptionWasEdited = false;
}

void dlgRoomProperties::slot_hashChanged(const QString& hash)
{
    // Counted as initGeneral() counts them when it enables the field, so the room's own game ID is not taken for another's
    const QSet<int> rooms = liveRoomIds();
    const int roomId = rooms.size() == 1 ? *rooms.cbegin() : 0;
    const int holder = hash.isEmpty() ? -1 : mpHost->mpMap->mpRoomDB->hashToRoomID.value(hash, -1);
    if (holder < 1 || holder == roomId) {
        label_hashWarning->hide();
        return;
    }
    //: Warning under Configure room's game ID field when the ID typed in already belongs to another room; %1 is that room's ID
    label_hashWarning->setText(tr("Room %1 has this game ID now and will lose it.").arg(holder));
    label_hashWarning->show();
}

void dlgRoomProperties::initUserData()
{
    QMap<QString, QSet<QString>> valuesByKey;
    QMap<QString, int> roomsByKey;
    int roomCount = 0;
    for (const int roomId : std::as_const(mRoomIds)) {
        const TRoom* room = liveRoom(roomId);
        if (!room) {
            continue;
        }
        ++roomCount;
        for (auto it = room->userData.cbegin(); it != room->userData.cend(); ++it) {
            if (!editedInUserDataTable(it.key())) {
                continue;
            }
            valuesByKey[it.key()].insert(it.value());
            ++roomsByKey[it.key()];
        }
    }

    mOriginalUserData.clear();
    for (auto it = valuesByKey.cbegin(); it != valuesByKey.cend(); ++it) {
        // A key only some of the rooms hold is as undecided as one they hold different values for
        const bool agreed = it.value().size() == 1 && roomsByKey.value(it.key()) == roomCount;
        mOriginalUserData.insert(it.key(), agreed ? std::optional<QString>(*it.value().cbegin()) : std::nullopt);
    }

    const QSignalBlocker blocker(tableWidget_userData);
    tableWidget_userData->setRowCount(0);
    for (auto it = mOriginalUserData.cbegin(); it != mOriginalUserData.cend(); ++it) {
        addUserDataRow(it.key(), it.value(), false);
    }
    tableWidget_userData->resizeColumnToContents(0);
}

void dlgRoomProperties::addUserDataRow(const QString& key, const std::optional<QString>& value, const bool keyEditable)
{
    const int row = tableWidget_userData->rowCount();
    tableWidget_userData->insertRow(row);

    auto* keyItem = new QTableWidgetItem(key);
    if (!keyEditable) {
        // Renaming a key would be a removal and an addition in one, which Remove and Add already say plainly
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
    }
    tableWidget_userData->setItem(row, 0, keyItem);

    auto* valueItem = new QTableWidgetItem(value.value_or(QString()));
    if (!value.has_value()) {
        valueItem->setData(scmStartedMixedRole, true);
        showUserDataValueMixed(valueItem, true);
    }
    tableWidget_userData->setItem(row, 1, valueItem);
}

void dlgRoomProperties::showUserDataValueMixed(QTableWidgetItem* item, const bool mixed)
{
    item->setData(scmMixedValueRole, mixed);
    if (mixed) {
        item->setText(multipleValuesPlaceholder);
    }
    QFont font = item->font();
    font.setItalic(mixed);
    item->setFont(font);
    item->setData(Qt::ForegroundRole, mixed ? QVariant(palette().placeholderText()) : QVariant());
}

QMap<QString, std::optional<QString>> dlgRoomProperties::userDataChanges() const
{
    QMap<QString, std::optional<QString>> changes;
    QSet<QString> keysLeft;
    for (int row = 0; row < tableWidget_userData->rowCount(); ++row) {
        const QTableWidgetItem* keyItem = tableWidget_userData->item(row, 0);
        const QTableWidgetItem* valueItem = tableWidget_userData->item(row, 1);
        if (!valueItem) {
            continue;
        }
        // A row listed from the rooms, whose key cannot be edited, is kept whatever its key: setRoomUserData()
        // accepts blank ones. Only an added row needs a key worth saving, and one typed with a space around it is
        // the key without
        const bool addedRow = keyItem && keyItem->flags().testFlag(Qt::ItemIsEditable);
        const QString key = !keyItem ? QString() : addedRow ? keyItem->text().trimmed() : keyItem->text();
        // The rows listed from the rooms come first, so an added row repeating a key is the one left out
        if (addedRow && (key.isEmpty() || !editedInUserDataTable(key) || keysLeft.contains(key))) {
            continue;
        }
        keysLeft.insert(key);
        if (valueItem->data(scmMixedValueRole).toBool()) {
            continue;
        }
        const auto original = mOriginalUserData.constFind(key);
        if (original != mOriginalUserData.cend() && original.value() == valueItem->text()) {
            continue;
        }
        changes.insert(key, valueItem->text());
    }
    for (auto it = mOriginalUserData.cbegin(); it != mOriginalUserData.cend(); ++it) {
        if (!keysLeft.contains(it.key())) {
            changes.insert(it.key(), std::nullopt);
        }
    }
    return changes;
}

void dlgRoomProperties::slot_addUserData()
{
    {
        const QSignalBlocker blocker(tableWidget_userData);
        addUserDataRow(QString(), QString(), true);
    }
    const int row = tableWidget_userData->rowCount() - 1;
    tableWidget_userData->setCurrentCell(row, 0);
    tableWidget_userData->editItem(tableWidget_userData->item(row, 0));
}

void dlgRoomProperties::slot_removeUserData()
{
    QList<int> rows;
    const auto selected = tableWidget_userData->selectionModel()->selectedRows();
    for (const QModelIndex& index : selected) {
        rows << index.row();
    }
    // From the bottom up, so removing a row does not move the ones still to go
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (const int row : std::as_const(rows)) {
        tableWidget_userData->removeRow(row);
    }
    updateUserDataKeyWarning();
}

void dlgRoomProperties::updateUserDataKeyWarning()
{
    QStringList fieldKeys;
    QStringList repeatedKeys;
    bool mapKey = false;
    // Read the way userDataChanges() reads them, which saves the first row of a key and leaves the others out
    QSet<QString> keysSeen;
    for (int row = 0; row < tableWidget_userData->rowCount(); ++row) {
        const QTableWidgetItem* keyItem = tableWidget_userData->item(row, 0);
        if (!keyItem) {
            continue;
        }
        // Only an added row's key can be typed; one listed from the rooms is shown only if the table edits it
        if (!keyItem->flags().testFlag(Qt::ItemIsEditable)) {
            keysSeen.insert(keyItem->text());
            continue;
        }
        const QString key = keyItem->text().trimmed();
        if (key.isEmpty()) {
            continue;
        }
        if (key.startsWith(scmMudletOwnedUserDataPrefix)) {
            mapKey = true;
        } else if (!editedInUserDataTable(key)) {
            if (!fieldKeys.contains(key)) {
                fieldKeys << key;
            }
        } else if (keysSeen.contains(key)) {
            if (!repeatedKeys.contains(key)) {
                repeatedKeys << key;
            }
        } else {
            keysSeen.insert(key);
        }
    }
    QStringList warnings;
    for (const QString& key : std::as_const(fieldKeys)) {
        //: Warning under the table on Configure room's User data tab; %1 is a key typed into a new row, such as "description", which has a field of its own on the General tab
        warnings << tr("\"%1\" is edited on the General tab, so this row will not be saved.").arg(key);
    }
    for (const QString& key : std::as_const(repeatedKeys)) {
        //: Warning under the table on Configure room's User data tab; %1 is a key typed into a new row that an earlier row already has
        warnings << tr("\"%1\" is already in the table, so this row will not be saved.").arg(key);
    }
    if (mapKey) {
        //: Warning under the table on Configure room's User data tab, when a key typed into a new row starts with room.ui_, which the map keeps for itself; do not translate room.ui_
        warnings << tr("Keys starting with room.ui_ belong to the map, so such a row will not be saved.");
    }
    label_userDataWarning->setText(warnings.join(QLatin1Char(' ')));
    label_userDataWarning->setVisible(!warnings.isEmpty());
}

void dlgRoomProperties::slot_userDataItemChanged(QTableWidgetItem* item)
{
    if (item && item->column() == 0) {
        updateUserDataKeyWarning();
        return;
    }
    if (!item || item->column() != 1 || !item->data(scmStartedMixedRole).toBool()) {
        return;
    }
    const QSignalBlocker blocker(tableWidget_userData);
    if (item->text().isEmpty()) {
        // Emptied again, as on the description field: the rooms keep their differing values
        showUserDataValueMixed(item, true);
    } else if (item->data(scmMixedValueRole).toBool()) {
        // Typed over, the cell now holds one value for every room rather than the placeholder
        showUserDataValueMixed(item, false);
    }
}

// The dialog is not modal, so a script may have deleted any of these rooms,
// or replaced one with a new room under the same id, while it was open
TRoom* dlgRoomProperties::liveRoom(const int roomId) const
{
    TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomId);
    if (!room || !mRoomSerials.contains(roomId) || room->serial() != mRoomSerials.value(roomId)) {
        return nullptr;
    }
    return room;
}

QSet<int> dlgRoomProperties::liveRoomIds() const
{
    QSet<int> result;
    for (const int roomId : std::as_const(mRoomIds)) {
        if (liveRoom(roomId)) {
            result.insert(roomId);
        }
    }
    return result;
}

void dlgRoomProperties::emitBorderPreview()
{
    // Apply current border settings directly to rooms for live preview
    for (const int roomId : std::as_const(mRoomIds)) {
        TRoom* room = liveRoom(roomId);
        if (!room) {
            continue;
        }
        if (mBorderColorWasChanged) {
            room->mBorderColor = selectedBorderColor;
        }
        if (mBorderThicknessWasChanged) {
            room->mBorderThickness = mBorderThickness;
        }
    }
    emit signal_preview_border(mRoomIds);
}

void dlgRoomProperties::restoreOriginalBorders()
{
    for (const int roomId : std::as_const(mRoomIds)) {
        TRoom* room = liveRoom(roomId);
        if (!room) {
            continue;
        }
        room->mBorderColor = mOriginalBorderColors.value(roomId);
        room->mBorderThickness = mOriginalBorderThicknesses.value(roomId);
    }
}

void dlgRoomProperties::reject()
{
    restoreOriginalBorders();
    emit signal_preview_border(mRoomIds);
    QDialog::reject();
}
