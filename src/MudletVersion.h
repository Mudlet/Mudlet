#ifndef MUDLET_MUDLETVERSION_H
#define MUDLET_MUDLETVERSION_H

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

#include <QString>

class QNetworkRequest;
class QUrl;

// Which build of Mudlet this is, and how it names itself to the outside world.
// Read on first use, so this is usable before any window exists.
namespace MudletVersion {
// The suffix CMake writes into :/app-build.txt - empty for an official release,
// "-ptb..." for a public test build, "-dev..." for everything else.
const QString& build();
const QString& scmVersion();
bool release();
bool publicTest();
bool development();
void setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request);
} // namespace MudletVersion

#endif // MUDLET_MUDLETVERSION_H
