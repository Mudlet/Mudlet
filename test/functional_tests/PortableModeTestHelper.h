#ifndef MUDLET_PORTABLEMODETESTHELPER_H
#define MUDLET_PORTABLEMODETESTHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

#include "utils.h"

// setupConfig() in mudlet.cpp consults portable.txt next to the executable
// before it consults XDG_CONFIG_HOME - but under an AppImage it looks beside
// the AppImage itself rather than the extracted binary, since findExecutableDir()
// prefers $APPIMAGE over QCoreApplication::applicationDirPath() there. A test
// that only checks applicationDirPath() misses a portable.txt an AppImage run
// would still honor, and ends up running against the real portable config
// instead of skipping - mirror findExecutableDir()'s resolution here rather
// than QCoreApplication::applicationDirPath() alone.
inline bool portableMarkerPresent()
{
    QString execDir = QCoreApplication::applicationDirPath();
    const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();
    if (systemEnvironment.contains(qsl("APPIMAGE"))) {
        execDir = QFileInfo(systemEnvironment.value(qsl("APPIMAGE"))).dir().path();
    }
    return QFileInfo::exists(qsl("%1/portable.txt").arg(execDir)) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
}

#endif // MUDLET_PORTABLEMODETESTHELPER_H
