#ifndef MUDLET_EVENTLOOPPUMP_H
#define MUDLET_EVENTLOOPPUMP_H

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

#include <functional>

class EventLoopPump
{
public:
    // Delivers Qt events for up to timeoutMs. True means stopCondition became
    // true, false means the time ran out. Not a nested QEventLoop::exec(): see
    // EventLoopPump.cpp for why exec() cannot be used here.
    [[nodiscard]] static bool pumpFor(int timeoutMs, const std::function<bool()>& stopCondition = {});
};

#endif // MUDLET_EVENTLOOPPUMP_H
