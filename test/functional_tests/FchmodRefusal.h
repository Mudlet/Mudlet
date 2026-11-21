#ifndef MUDLET_FCHMODREFUSAL_H
#define MUDLET_FCHMODREFUSAL_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

// Linked only into binaries whose CMakeLists.txt also defines MUDLET_FCHMOD_REFUSAL; a case
// without it QSKIPs, so add both
void setRefuseOwnerOnlyOnOpenFiles(bool refuse);
// How many fchmod() calls have been refused so far, so a case can show its refusal was reached
int ownerOnlyRefusals();

#endif // MUDLET_FCHMODREFUSAL_H
