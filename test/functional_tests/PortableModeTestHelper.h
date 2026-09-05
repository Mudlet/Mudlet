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

#include "MudletPaths.h"

// A portable.txt beside the executable - or beside the AppImage, when running
// from one - or in ~/.config/mudlet relocates the config root, so a test that
// would otherwise touch the real one has to skip
inline bool portableMarkerPresent()
{
    return MudletPaths::resolveConfigRoot(MudletPaths::executableDir()).portable;
}

#endif // MUDLET_PORTABLEMODETESTHELPER_H
