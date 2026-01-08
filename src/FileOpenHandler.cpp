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

#include "FileOpenHandler.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

FileOpenHandler::FileOpenHandler(QObject* parent) : QObject(parent)
{
    QCoreApplication::instance()->installEventFilter(this);
}

bool FileOpenHandler::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
        if (!mudlet::self()) {
            qWarning() << "FileOpenHandler::eventFilter() - mudlet instance not available, cannot process file open event";
            return false;
        }
        MudletInstanceCoordinator* instanceCoordinator = mudlet::self()->getInstanceCoordinator();
        if (!instanceCoordinator) {
            qWarning() << "FileOpenHandler::eventFilter() - Instance coordinator not available, cannot process file open event";
            return false;
        }
        const QString absPath = QDir(openEvent->file()).absolutePath();
        instanceCoordinator->queuePackage(absPath);
        instanceCoordinator->installPackagesLocally();
        return true;
    }
    return QObject::eventFilter(obj, event);
}

