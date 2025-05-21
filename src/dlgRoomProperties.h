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

#include "Host.h"

#include "pre_guard.h"
#include "ui_room_properties.h"
#include "post_guard.h"


class dlgRoomProperties : public QDialog, public Ui::room_properties
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgRoomProperties)
    explicit dlgRoomProperties(Host*, QWidget* parent = nullptr);
    void init(
        QHash<QString, int> usedNames,
        QHash<int, int>& pColors,
        QHash<QString, int>& pSymbols,
        QHash<int, int>& pWeights,
        QHash<bool, int> lockStatus,
        QSet<TRoom*>& pRooms);
    void accept() override;

signals:
    void signal_save_symbol(
        bool changeName, QString newName,
        bool mChangeRoomColor, int mRoomColorNumber,
        bool changeSymbol, QString newSymbol,
        bool changeSymbolColor, QColor newSymbolColor,
        bool changeWeight, int newWeight,
        bool changeLockStatus, std::optional<bool> newLockStatus,
        QSet<TRoom*> mpRooms);

private:
    QColor backgroundBasedColor(QColor);
    QColor defaultSymbolColor();
    QString getNewSymbol();
    void initSymbolInstructions();
    QStringList getComboBoxSymbolItems();
    QFont getFontForPreview(QString);

    int getNewWeight();
    void initWeightInstructions();
    QStringList getComboBoxWeightItems();
    void initLockInstructions();

    Host* mpHost = nullptr;
    QSet<TRoom*> mpRooms;
    QHash<QString, int> mpSymbols;
    QHash<int, int> mpWeights;
    QColor selectedSymbolColor;
    QColor mRoomColor;
    int mRoomColorNumber = -1;
    bool mChangeRoomColor = false;
    QString multipleValuesPlaceholder = tr("(Multiple values...)");

private slots:
    void slot_openSymbolColorSelector();
    void slot_symbolColorSelected(const QColor&);
    void slot_updatePreview();
    void slot_resetSymbolColor();

    void slot_selectRoomColor(QListWidgetItem*);
    void slot_defineNewColor();
    void slot_openRoomColorSelector();

    void slot_symbolComboBoxItemChanged(const int);
    void slot_weightComboBoxItemChanged(const int);
};

#endif // MUDLET_DLGROOMPROPERTIES_H
