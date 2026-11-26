#ifndef MUDLET_DLGROOMPROPERTIES_H
#define MUDLET_DLGROOMPROPERTIES_H

/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022-2023, 2025 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

#include "ui_room_properties.h"

#include <QColor>
#include <QMap>

#include <optional>

class Host;
class QStyledItemDelegate;
class TRoom;

namespace uiDesign {
class ColourValue;
}

class dlgRoomProperties : public QDialog, public Ui::room_properties
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgRoomProperties)
    explicit dlgRoomProperties(Host*, QWidget* parent = nullptr);
    ~dlgRoomProperties() override;
    void
    init(QHash<QString, int> usedNames, QHash<int, int>& pColors, QHash<QString, int>& pSymbols, QHash<int, int>& pWeights, QHash<bool, int> lockStatus, int hiddenRoomCount, const QSet<int>& roomIds);
    void accept() override;
    void reject() override;

signals:
    void signal_save_symbol(bool changeName,
                            QString newName,
                            bool mChangeRoomColor,
                            int mRoomColorNumber,
                            bool changeSymbol,
                            QString newSymbol,
                            bool changeSymbolColor,
                            QColor newSymbolColor,
                            bool changeWeight,
                            int newWeight,
                            bool changeLockStatus,
                            std::optional<bool> newLockStatus,
                            bool changeHiddenStatus,
                            std::optional<bool> newHiddenStatus,
                            bool changeBorderColor,
                            QColor newBorderColor,
                            bool changeBorderThickness,
                            int newBorderThickness,
                            QSet<int> roomIds);
    void signal_preview_border(QSet<int> roomIds);
    // Only the keys the player changed: a value to set, or std::nullopt to remove the key
    void signal_save_userData(QMap<QString, std::optional<QString>> changes, QSet<int> roomIds);
    // Only for a single room, and only when its game ID was changed; an empty hash clears it
    void signal_save_hash(int roomId, QString hash);
    // Only when an area was picked that not every room is in already
    void signal_save_area(int areaId, QSet<int> roomIds);

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
    void changeEvent(QEvent* pEvent) override;
    void showEvent(QShowEvent* pEvent) override;

private:
    QColor backgroundBasedColor(QColor);
    void applyRoomPropertiesShellStyle();
    void spaceTheSections();
    // The colour boxes, the symbol field and the border width stand one over another, so they share one width
    void alignValueBoxes();
    void keepTheTabsRoomForThePages();
    bool resetOffered(const QColor& chosen) const;
    bool borderColoursDiffer() const;
    void showBorderColour();
    void showRoomColour();
    QColor defaultSymbolColor();
    QString getNewSymbol();
    QStringList getComboBoxSymbolItems();

    int getNewWeight();
    QStringList getComboBoxWeightItems();
    void initLockAndHiddenWords();
    void emitBorderPreview();
    void restoreOriginalBorders();
    void initGeneral();
    void initUserData();
    void addUserDataRow(const QString& key, const std::optional<QString>& value, const bool keyEditable);
    void showUserDataValueMixed(QTableWidgetItem*, const bool mixed);
    QMap<QString, std::optional<QString>> userDataChanges() const;
    // Says which keys typed into added rows are kept elsewhere and so will not be saved from the table
    void updateUserDataKeyWarning();
    void showChosenValue();
    void showValueLabel();
    void showUserDataEmpty();
    void fitScriptData();
    void fitKeyColumn();
    void focusValueBox();
    TRoom* liveRoom(const int) const;
    QSet<int> liveRoomIds() const;

    Host* mpHost = nullptr;
    uiDesign::ColourValue* mpRoomColour = nullptr;
    uiDesign::ColourValue* mpSymbolColour = nullptr;
    uiDesign::ColourValue* mpBorderColour = nullptr;
    QStyledItemDelegate* mpValueDelegate = nullptr;
    QLabel* mpUserDataEmpty = nullptr;
    QSet<int> mRoomIds;
    QHash<int, quint64> mRoomSerials;
    QHash<QString, int> mpSymbols;
    QHash<int, int> mpWeights;
    QColor selectedSymbolColor;
    QColor mRoomColor;
    int mRoomColorNumber = -1;
    bool mRoomColorsDiffer = false;
    bool mChangeRoomColor = false;
    // The rooms have different names, so an empty name field leaves them all as they are
    bool mNamesDiffer = false;
    bool mSymbolColorWasChanged = false;
    QColor selectedBorderColor;
    int mBorderThickness = 0;
    bool mBorderColorWasChanged = false;
    // An opacity set for rooms that differ in colour, which each room takes with its own colour
    std::optional<int> mBorderAlphaOnly;
    bool mBorderThicknessWasChanged = false;
    QHash<int, QColor> mOriginalBorderColors;
    QHash<int, int> mOriginalBorderThicknesses;
    // Each key the selected rooms hold, with its value, or std::nullopt where they do not all hold the same one
    QMap<QString, std::optional<QString>> mOriginalUserData;
    QString mOriginalHash;
    // The rooms' area, or std::nullopt where they are in different ones
    std::optional<int> mOriginalAreaId;
    // What the chosen row held when it was chosen, which decides how an edit in the value box is written back
    QString mShownValue;
    // The chosen row is a JSON list or table the value box shows indented
    bool mShownAsJson = false;
    bool mKeyColumnFitted = false;
    bool mSizedForShow = false;
    bool mFittingScriptData = false;
    QString multipleValuesPlaceholder = tr("(Multiple values...)");

private slots:
    void slot_openSymbolColorSelector();
    void slot_symbolColorSelected(const QColor&);
    void slot_updatePreview();
    void slot_resetSymbolColor();

    void slot_defineNewColor();
    void slot_openRoomColorSelector();

    void slot_symbolComboBoxItemChanged(const int);
    void slot_weightComboBoxItemChanged(const int);

    void slot_openBorderColorSelector();
    void slot_borderColorSelected(const QColor&);
    void slot_borderOpacitySelected(const QColor&);
    void slot_resetBorderColor();
    void slot_borderThicknessChanged(int);

    void slot_addUserData();
    void slot_removeUserData();
    void slot_userDataItemChanged(QTableWidgetItem*);
    void slot_valueBoxEdited();
    void slot_hashChanged(const QString&);
};

#endif // MUDLET_DLGROOMPROPERTIES_H
