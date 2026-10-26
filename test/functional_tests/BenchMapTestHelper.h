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

#include <QString>

// The map benchmarks time TMap::restore(), which reads only the binary format,
// so an XML map - the Achaea fixture CI/pgo-train.sh trains on - is imported and
// saved in that format under dir first. Empty if either step fails.
inline QString binaryBenchMap(Host* host, const QString& mapPath, const QString& dir)
{
    if (!mapPath.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive)) {
        return mapPath;
    }
    const QString binaryPath = dir + QLatin1String("/bench-map.dat");
    if (!host->importMapFile(mapPath) || !host->saveMapFile(binaryPath)) {
        return {};
    }
    return binaryPath;
}

#endif // MUDLET_BENCHMAPTESTHELPER_H
