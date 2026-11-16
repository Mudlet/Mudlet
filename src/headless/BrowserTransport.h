#ifndef MUDLET_BROWSERTRANSPORT_H
#define MUDLET_BROWSERTRANSPORT_H

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

class THeadlessSession;

// Whether the page passed Qt's loader a mudletTransport, the connection it makes to the game in place of sockets
bool pageSuppliesTransport();
// Hands the session's game connection to the page's mudletTransport; see "The game connection" in docs/headless.md
void bridgeTransportToPage(THeadlessSession* pSession);
// Calls the page's mudletConfigSaved(), if it passed Qt's loader one, on each configSaved; see "Keeping the profiles"
// in docs/headless.md
void bridgeConfigSavesToPage(THeadlessSession* pSession);

#endif // MUDLET_BROWSERTRANSPORT_H
