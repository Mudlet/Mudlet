#ifndef MUDLET_TSCROLLBOX_H
#define MUDLET_TSCROLLBOX_H

/***************************************************************************
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
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

#include "pre_guard.h"
#include <QScrollArea>
#include <QPointer>
#include "post_guard.h"

class Host;

class TScrollBox : public QScrollArea
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TScrollBox)
    explicit TScrollBox(Host* pH, QWidget* pW = nullptr);
    QPointer<Host> mpHost;


private:
};


class TScrollBoxWidget : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TScrollBoxWidget)
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 13, 0))
    Q_DISABLE_MOVE(TScrollBoxWidget)
#endif
    explicit TScrollBoxWidget(QWidget* pW = nullptr);
    ~TScrollBoxWidget();
    void childEvent(QChildEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;
};

#endif // MUDLET_TSCROLLBOX_H
