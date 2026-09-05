#ifndef MUDLET_MUDLETARCHIVE_H
#define MUDLET_MUDLETARCHIVE_H

/***************************************************************************
 *   Copyright (C) 2013-2026 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2011-2021 by Vadim Peretokin - vperetokin@gmail.com     *
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

class QDir;
class QString;

namespace MudletArchive {
// Unpacks archivePath into destination, creating any folders the archive needs
// through tmpDir. Called from a worker thread, so nothing in here may touch the
// UI.
bool unzip(const QString& archivePath, const QString& destination, const QDir& tmpDir);
} // namespace MudletArchive

#endif // MUDLET_MUDLETARCHIVE_H
