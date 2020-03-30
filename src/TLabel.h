#ifndef MUDLET_TLABEL_H
#define MUDLET_TLABEL_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "TEvent.h"

#include "pre_guard.h"
#include <QLabel>
#include <QPointer>
#include <QString>
#include "post_guard.h"

class Host;

class QMouseEvent;

class TLabel : public QLabel
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TLabel)
    TLabel(Host* pH, QWidget* pW = nullptr);
    void setClick(void *func);
    void setDoubleClick(void* func);
    void setRelease(void* func);
    void setMove(void* func);
    void setWheel(void* func);
    void setEnter(void* func);
    void setLeave(void* func);
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void enterEvent(QEvent*) override;
    void setClickThrough(bool clickthrough);

    bool forwardEventToMapper(QEvent*);

    QPointer<Host> mpHost;
    void* mClick;
    void* mDoubleClick;
    void* mRelease;
    void* mMove;
    void* mWheel;
    void* mEnter;
    void* mLeave;
    bool mHaveClick = false;
    bool mHaveDoubleClick = false;
    bool mHaveRelease = false;
    bool mHaveMove = false;
    bool mHaveWheel = false;
    bool mHaveEnter = false;
    bool mHaveLeave = false;

};

#endif // MUDLET_TLABEL_H
