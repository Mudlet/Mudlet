#ifndef MUDLET_BENCHMAPTESTHELPER_H
#define MUDLET_BENCHMAPTESTHELPER_H

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

#include "Host.h"
#include "TMap.h"
#include "utils.h"

#include <QString>

// TMap::restore() reads only the binary format, so an XML map is imported, saved
// as one under dir and cleared - restore() adds to the map rather than replacing
// it, which is why Host::loadMapFile() clears first. Empty on failure, with errMsg set.
inline QString binaryBenchMap(Host* host, const QString& mapPath, const QString& dir, QString& errMsg)
{
    if (!mapPath.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        return mapPath;
    }
    if (!host->importMapFile(mapPath, &errMsg)) {
        return {};
    }
    const QString binaryPath = qsl("%1/bench-map.dat").arg(dir);
    if (!host->saveMapFile(binaryPath)) {
        errMsg = qsl("could not save it as %1").arg(binaryPath);
        return {};
    }
    host->mpMap->mapClear();
    return binaryPath;
}

#endif // MUDLET_BENCHMAPTESTHELPER_H
