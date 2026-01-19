#ifndef MUDLET_TEASYBUTTONBAR_H
#define MUDLET_TEASYBUTTONBAR_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2019 by Stephen Lyons - slysven@virginmedia.com   *
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


#include <QWidget>

#include <list>

class TFlipButton;
class TAction;

class QGridLayout;


class TEasyButtonBar : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TEasyButtonBar)
    TEasyButtonBar(TAction*, QString, QWidget* pW = nullptr);
    void addButton(TFlipButton* pW);
    void setVerticalOrientation() { mVerticalOrientation = true; }
    void setHorizontalOrientation() { mVerticalOrientation = false; }
    void clear();
    void finalize();
    void recordMove() { mRecordMove = true; }

    TAction* mpTAction = nullptr;

public slots:
    void slot_pressed(bool);

private:
    bool mVerticalOrientation = false;
    QWidget* mpWidget = nullptr;
    bool mRecordMove = false;
    QGridLayout* mpLayout = nullptr;
    int mItemCount = 0;
    std::list<TFlipButton*> mButtonList;
};

#endif // MUDLET_TEASYBUTTONBAR_H
