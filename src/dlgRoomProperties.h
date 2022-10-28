#ifndef MUDLET_DLGROOMPROPERTIES_H
#define MUDLET_DLGROOMPROPERTIES_H

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
        bool changeLockStatus, bool newLockStatus,
        QSet<TRoom*> mpRooms);

private:
    QColor backgroundBasedColor(QColor);
    QColor defaultSymbolColor();
    QString getNewSymbol();
    void initSymbolInstructionLabel();
    QStringList getComboBoxSymbolItems();
    QFont getFontForPreview(QString);
    QString getComboInput(QString);

    int getNewWeight();
    void initWeightInstructionLabel();
    QStringList getComboBoxWeightItems();

    Host* mpHost;
    QSet<TRoom*> mpRooms;
    QHash<QString, int> mpSymbols;
    QHash<int, int> mpWeights;
    QColor selectedSymbolColor;
    QColor mRoomColor;
    int mRoomColorNumber;
    bool mChangeRoomColor = false;;
    QString multipleValuesPlaceholder = tr("(Multiple values...)");

private slots:
    void slot_openSymbolColorSelector();
    void slot_symbolColorSelected(const QColor&);
    void slot_updatePreview();
    void slot_resetSymbolColor();
    void slot_openRoomColorSelector();
};

#endif // MUDLET_DLGROOMPROPERTIES_H
