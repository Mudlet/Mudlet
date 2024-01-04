/***************************************************************************
 *   Copyright (C) 2024-2024 by Adam Robinson - seldon1951@hotmail.com     *
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

#ifndef FILEOPENHANDLER_H
#define FILEOPENHANDLER_H

#include <mudlet.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFileOpenEvent>

class FileOpenHandler : public QObject {
    Q_OBJECT

public:
    explicit FileOpenHandler(QObject *parent = nullptr) : QObject(parent) {
        QCoreApplication::instance()->installEventFilter(this);
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            Q_ASSERT(mudlet::self());
            MudletServer* server = mudlet::self()->getServer();
            const QString absPath = QDir(openEvent->file()).absolutePath();
            server->queuePackage(absPath);
            server->installPackagesLocally();
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

#endif //FILEOPENHANDLER_H
