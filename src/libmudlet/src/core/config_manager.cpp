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

#include "config_manager.h"
#include "utils/logger.h"
#include "utils/file_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace mudlet {

ConfigManager::ConfigManager() {
    setDefaults();
}

void ConfigManager::setDefaults() {
    // Set reasonable defaults for libmudlet
    mGlobalConfig["encoding"] = "UTF-8";
    mGlobalConfig["buffer_max_lines"] = "10000";
    mGlobalConfig["wrap_width"] = "100";
    mGlobalConfig["connect_timeout"] = "30";
    mGlobalConfig["network_retries"] = "3";
    mGlobalConfig["log_level"] = "Info";
    mGlobalConfig["script_timeout"] = "5000";  // milliseconds
    mGlobalConfig["trigger_limit"] = "1000";
    mGlobalConfig["alias_limit"] = "1000";
    mGlobalConfig["timer_limit"] = "1000";
    mGlobalConfig["keybind_limit"] = "1000";
}

void ConfigManager::setGlobalConfig(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mMutex);
    mGlobalConfig[key] = value;
    Logger::debug("Set global config: {} = {}", key, value);
}

std::string ConfigManager::getGlobalConfig(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mGlobalConfig.find(key);
    return (it != mGlobalConfig.end()) ? it->second : defaultValue;
}

bool ConfigManager::hasGlobalConfig(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mGlobalConfig.find(key) != mGlobalConfig.end();
}

void ConfigManager::removeGlobalConfig(const std::string& key) {
    std::lock_guard<std::mutex> lock(mMutex);
    mGlobalConfig.erase(key);
    Logger::debug("Removed global config: {}", key);
}

void ConfigManager::setSessionConfig(const std::string& session, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mMutex);
    mSessionConfigs[session][key] = value;
    Logger::debug("Set session config for '{}': {} = {}", session, key, value);
}

std::string ConfigManager::getSessionConfig(const std::string& session, const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto sessionIt = mSessionConfigs.find(session);
    if (sessionIt == mSessionConfigs.end()) {
        return defaultValue;
    }
    
    auto configIt = sessionIt->second.find(key);
    return (configIt != sessionIt->second.end()) ? configIt->second : defaultValue;
}

bool ConfigManager::hasSessionConfig(const std::string& session, const std::string& key) const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto sessionIt = mSessionConfigs.find(session);
    if (sessionIt == mSessionConfigs.end()) {
        return false;
    }
    
    return sessionIt->second.find(key) != sessionIt->second.end();
}

void ConfigManager::removeSessionConfig(const std::string& session, const std::string& key) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto sessionIt = mSessionConfigs.find(session);
    if (sessionIt != mSessionConfigs.end()) {
        sessionIt->second.erase(key);
        Logger::debug("Removed session config for '{}': {}", session, key);
    }
}

void ConfigManager::clearSessionConfig(const std::string& session) {
    std::lock_guard<std::mutex> lock(mMutex);
    mSessionConfigs.erase(session);
    Logger::debug("Cleared all config for session '{}'", session);
}

bool ConfigManager::loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::warning("Could not open config file: {}", filename);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mMutex);
    mGlobalConfig.clear();
    mSessionConfigs.clear();
    setDefaults();
    
    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section header [section_name]
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key=value pairs
        auto equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Unescape value
            value = unescape(value);
            
            if (currentSection.empty()) {
                mGlobalConfig[key] = value;
            } else {
                mSessionConfigs[currentSection][key] = value;
            }
        }
    }
    
    Logger::info("Loaded configuration from: {}", filename);
    return true;
}

bool ConfigManager::saveConfig(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Logger::error("Could not open config file for writing: {}", filename);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    // Write global configuration
    file << "# LibMudlet Configuration File\n";
    file << "# Global settings\n\n";
    
    for (const auto& [key, value] : mGlobalConfig) {
        file << key << "=" << escape(value) << "\n";
    }
    
    // Write session configurations
    for (const auto& [session, config] : mSessionConfigs) {
        file << "\n[" << session << "]\n";
        for (const auto& [key, value] : config) {
            file << key << "=" << escape(value) << "\n";
        }
    }
    
    Logger::info("Saved configuration to: {}", filename);
    return true;
}

void ConfigManager::loadDefaultConfig() {
    std::lock_guard<std::mutex> lock(mMutex);
    mGlobalConfig.clear();
    mSessionConfigs.clear();
    setDefaults();
    Logger::info("Loaded default configuration");
}

std::vector<std::string> ConfigManager::getConfigKeys() const {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<std::string> keys;
    keys.reserve(mGlobalConfig.size());
    
    for (const auto& [key, value] : mGlobalConfig) {
        keys.push_back(key);
    }
    
    return keys;
}

std::vector<std::string> ConfigManager::getSessionConfigKeys(const std::string& session) const {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<std::string> keys;
    
    auto sessionIt = mSessionConfigs.find(session);
    if (sessionIt != mSessionConfigs.end()) {
        keys.reserve(sessionIt->second.size());
        for (const auto& [key, value] : sessionIt->second) {
            keys.push_back(key);
        }
    }
    
    return keys;
}

std::string ConfigManager::escape(const std::string& value) const {
    std::string result;
    result.reserve(value.length() + 10);
    
    for (char c : value) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\\': result += "\\\\"; break;
            case '\"': result += "\\\""; break;
            default: result += c; break;
        }
    }
    
    return result;
}

std::string ConfigManager::unescape(const std::string& value) const {
    std::string result;
    result.reserve(value.length());
    
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '\\' && i + 1 < value.length()) {
            switch (value[i + 1]) {
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '\"': result += '\"'; ++i; break;
                default: result += value[i]; break;
            }
        } else {
            result += value[i];
        }
    }
    
    return result;
}

} // namespace mudlet