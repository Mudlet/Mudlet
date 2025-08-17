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

#include "logger.h"

namespace mudlet {

// Static member initialization
LogLevel Logger::sCurrentLevel = LogLevel::Info;
bool Logger::sInitialized = false;

void Logger::initialize() {
    if (sInitialized) {
        return;
    }
    
    sCurrentLevel = LogLevel::Info;
    sInitialized = true;
}

void Logger::shutdown() {
    sInitialized = false;
}

void Logger::setLevel(LogLevel level) {
    sCurrentLevel = level;
}

LogLevel Logger::getLevel() {
    return sCurrentLevel;
}

const char* Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

} // namespace mudlet