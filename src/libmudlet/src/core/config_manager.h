#ifndef LIBMUDLET_CONFIG_MANAGER_H
#define LIBMUDLET_CONFIG_MANAGER_H

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
#include <unordered_map>
#include <mutex>

namespace mudlet {

/**
 * @brief Configuration manager for libmudlet
 * 
 * Handles both global configuration and session-specific settings.
 * Thread-safe for concurrent access from multiple sessions.
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;
    
    // Global configuration
    void setGlobalConfig(const std::string& key, const std::string& value);
    std::string getGlobalConfig(const std::string& key, const std::string& defaultValue = "") const;
    bool hasGlobalConfig(const std::string& key) const;
    void removeGlobalConfig(const std::string& key);
    
    // Session-specific configuration
    void setSessionConfig(const std::string& session, const std::string& key, const std::string& value);
    std::string getSessionConfig(const std::string& session, const std::string& key, const std::string& defaultValue = "") const;
    bool hasSessionConfig(const std::string& session, const std::string& key) const;
    void removeSessionConfig(const std::string& session, const std::string& key);
    void clearSessionConfig(const std::string& session);
    
    // Configuration persistence
    bool loadConfig(const std::string& filename);
    bool saveConfig(const std::string& filename) const;
    void loadDefaultConfig();
    
    // Utility methods
    std::vector<std::string> getConfigKeys() const;
    std::vector<std::string> getSessionConfigKeys(const std::string& session) const;
    
private:
    mutable std::mutex mMutex;
    std::unordered_map<std::string, std::string> mGlobalConfig;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> mSessionConfigs;
    
    void setDefaults();
    std::string escape(const std::string& value) const;
    std::string unescape(const std::string& value) const;
};

} // namespace mudlet

#endif // LIBMUDLET_CONFIG_MANAGER_H