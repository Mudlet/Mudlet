#ifndef LIBMUDLET_CORE_IMPL_H
#define LIBMUDLET_CORE_IMPL_H

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

#include "mudlet/core.h"
#include "session_impl.h"
#include "config_manager.h"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace mudlet {

/**
 * @brief Core implementation for libmudlet
 */
class CoreImpl : public Core {
public:
    CoreImpl();
    ~CoreImpl() override;

    // Core interface implementation
    std::shared_ptr<Session> createSession(const std::string& name) override;
    std::shared_ptr<Session> getSession(const std::string& name) override;
    bool removeSession(const std::string& name) override;
    std::vector<std::string> getSessionNames() const override;
    
    void setConfig(const std::string& key, const std::string& value) override;
    std::string getConfig(const std::string& key, const std::string& defaultValue = "") const override;
    
    void processEvents() override;

private:
    mutable std::mutex mSessionsMutex;
    std::unordered_map<std::string, std::shared_ptr<SessionImpl>> mSessions;
    std::unique_ptr<ConfigManager> mConfigManager;
    
    // Global state
    bool mInitialized = false;
    
    void initializeCore();
    void shutdownCore();
};

} // namespace mudlet

#endif // LIBMUDLET_CORE_IMPL_H