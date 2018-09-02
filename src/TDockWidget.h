#ifndef MUDLET_TDOCKWIDGET_H
#define MUDLET_TDOCKWIDGET_H
/***************************************************************************
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
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

#include "mudlet.h"
#include "Host.h"
#include "TConsole.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QDockWidget>
#include <QPointer>
#include <QString>
#include "post_guard.h"

// TDockWidget contains helpers for User Windows QDockWidget.
class TDockWidget : public QDockWidget {
public:
    QString widgetConsoleName;
    bool hasLayoutAlready;

    TDockWidget(Host * , const QString &);

protected:
    void closeEvent(QCloseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;

private:
    QPointer<Host> mpHost;
};

#endif // MUDLET_TDOCKWIDGET_H
