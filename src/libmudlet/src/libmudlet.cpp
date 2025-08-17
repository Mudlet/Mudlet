/***************************************************************************
 *   Copyright (C) 2025 by Rishi Mondal - mavrickrishi@gmail.com          *
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

#include "mudlet/libmudlet.h"
#include "utils/logger.h"

namespace mudlet {

void initialize() {
    Logger::initialize();
    Logger::info("LibMudlet {} initialized", LIBMUDLET_VERSION_STRING);
}

void shutdown() {
    Logger::info("LibMudlet {} shutting down", LIBMUDLET_VERSION_STRING);
    Logger::shutdown();
}

const char* getVersion() {
    return LIBMUDLET_VERSION_STRING;
}

} // namespace mudlet
