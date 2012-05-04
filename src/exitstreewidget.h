#ifndef EXITSTREEWIDGET_H
#define EXITSTREEWIDGET_H
/***************************************************************************
 *   Copyright (C) 2012 by Vadim Peretokin                                 *
 *   vadim.peretokin@mudlet.org                                            *
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

#include <QTreeWidget>
#include <QDebug>

class Host;

class ExitsTreeWidget : public QTreeWidget
{
Q_OBJECT

public:
        ExitsTreeWidget( QWidget * pW );
        void keyPressEvent ( QKeyEvent * event );
private:

signals:

public slots:
};


#endif // EXITSTREEWIDGET_H
