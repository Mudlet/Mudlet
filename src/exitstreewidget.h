#ifndef MUDLET_EXITSTREEWIDGET_H
#define MUDLET_EXITSTREEWIDGET_H

/***************************************************************************
 *   Copyright (C) 2012 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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
#include <QTreeWidget>
#include "post_guard.h"


class ExitsTreeWidget : public QTreeWidget
{
    Q_OBJECT

    Q_DISABLE_COPY(ExitsTreeWidget)

public:
    ExitsTreeWidget(QWidget* pW);
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // MUDLET_EXITSTREEWIDGET_H
