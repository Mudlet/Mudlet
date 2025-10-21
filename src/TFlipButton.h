#ifndef MUDLET_TFLIPBUTTON_H
#define MUDLET_TFLIPBUTTON_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2022 by Stephen Lyons - slysven@virginmedia.com   *
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


#include <QPointer>
#include <QPushButton>

class Host;
class TAction;

class TFlipButton : public QPushButton
{
public:
    TFlipButton(TAction*, Host*);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation);

    bool mirrored() const;
    void setMirrored(bool);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QStyleOptionButton getStyleOption() const;

    TAction* mpTAction = nullptr;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int mID = 0;
    QPointer<Host> mpHost;
    Qt::Orientation mOrientation = Qt::Horizontal;
    bool mMirrored = false;
};

#endif // MUDLET_TFLIPBUTTON_H
