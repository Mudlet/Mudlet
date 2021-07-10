#ifndef MUDLET_DLGROOMSYMBOL_H
#define MUDLET_DLGROOMSYMBOL_H

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

#include "Host.h"

#include "pre_guard.h"
#include "ui_room_symbol.h"
#include "post_guard.h"


class dlgRoomSymbol : public QDialog, public Ui::room_symbol
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgRoomSymbol)
    explicit dlgRoomSymbol(Host*, QWidget* parent = nullptr);
    void init(QHash<QString, int>& pSymbols, QSet<TRoom*>& pRooms);
    void accept() override;

signals:
    void signal_save_symbol(QString symbol, QColor color, QSet<TRoom*> rooms);

private:
    QColor backgroundBasedColor(QColor);
    QColor defaultColor();
    QString getNewSymbol();
    void initInstructionLabel();
    QStringList getComboBoxItems();
    QFont getFontForPreview(QString);

    Host* mpHost;
    QSet<TRoom*> mpRooms;
    QHash<QString, int> mpSymbols;
    QColor selectedColor = nullptr;
    QColor previewColor = nullptr;
    QColor roomColor;

private slots:
    void openColorSelector();
    void currentColorChanged(const QColor&);
    void colorSelected(const QColor&);
    void colorRejected();
    void updatePreview();
    void resetColor();
};

#endif // MUDLET_DLGROOMSYMBOL_H
