#ifndef LIBMUDLET_LOGGER_H
#define LIBMUDLET_LOGGER_H

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

#include <string>
#include <memory>
#include <iostream>
#include <sstream>

namespace mudlet {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

/**
 * @brief Simple logging utility for libmudlet
 * 
 * Provides basic logging functionality without external dependencies.
 * Can be extended to support different output formats and destinations.
 */
class Logger {
public:
    static void initialize();
    static void shutdown();
    
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    
    template<typename... Args>
    static void debug(const std::string& format, Args&&... args) {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(const std::string& format, Args&&... args) {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warning(const std::string& format, Args&&... args) {
        log(LogLevel::Warning, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const std::string& format, Args&&... args) {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void critical(const std::string& format, Args&&... args) {
        log(LogLevel::Critical, format, std::forward<Args>(args)...);
    }

private:
    static LogLevel sCurrentLevel;
    static bool sInitialized;
    
    template<typename... Args>
    static void log(LogLevel level, const std::string& format, Args&&... args) {
        if (!sInitialized || level < sCurrentLevel) {
            return;
        }
        
        // Simple string replacement for {} placeholders
        std::string message = format;
        if constexpr (sizeof...(args) > 0) {
            message = simpleFormat(format, std::forward<Args>(args)...);
        }
        
        // Get level string
        const char* levelStr = getLevelString(level);
        
        // Output the log message
        auto& output = (level >= LogLevel::Error) ? std::cerr : std::cout;
        output << "[" << levelStr << "] " << message << std::endl;
    }
    
    template<typename T>
    static std::string simpleFormat(const std::string& format, T&& value) {
        std::string result = format;
        size_t pos = result.find("{}");
        if (pos != std::string::npos) {
            std::ostringstream oss;
            oss << value;
            result.replace(pos, 2, oss.str());
        }
        return result;
    }
    
    template<typename T, typename... Args>
    static std::string simpleFormat(const std::string& format, T&& value, Args&&... args) {
        std::string result = format;
        size_t pos = result.find("{}");
        if (pos != std::string::npos) {
            std::ostringstream oss;
            oss << value;
            result.replace(pos, 2, oss.str());
            return simpleFormat(result, std::forward<Args>(args)...);
        }
        return result;
    }
    
    static const char* getLevelString(LogLevel level);
};

} // namespace mudlet

#endif // LIBMUDLET_LOGGER_H