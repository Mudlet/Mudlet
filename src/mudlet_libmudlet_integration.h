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

#ifndef MUDLET_LIBMUDLET_INTEGRATION_H
#define MUDLET_LIBMUDLET_INTEGRATION_H

/**
 * @file mudlet_libmudlet_integration.h
 * @brief Integration header for existing Mudlet code to use libmudlet
 * 
 * This header provides compatibility macros and includes for gradually
 * migrating existing Mudlet Qt code to use libmudlet backends.
 */

// Include libmudlet
#include "libmudlet/include/mudlet/libmudlet.h"
#include "qt-adapter/libmudlet_qt_adapter.h"

// Forward declarations for existing classes
class Host;
class TTrigger;
class TAlias; 
class TTimer;
class TBuffer;

/**
 * @brief Feature flags for gradual migration
 */
namespace MudletFeatureFlags {
    extern bool UseLibMudletTriggers;
    extern bool UseLibMudletAliases;
    extern bool UseLibMudletTimers;
    extern bool UseLibMudletTextBuffer;
    extern bool UseLibMudletNetworking;
}

/**
 * @brief Integration helper for Host class
 * 
 * Provides methods to bridge existing Host functionality
 * with new libmudlet backend.
 */
class HostLibMudletBridge {
public:
    explicit HostLibMudletBridge(Host* host);
    ~HostLibMudletBridge();
    
    // Initialize libmudlet backend for this host
    bool initializeLibMudlet();
    void shutdownLibMudlet();
    
    // Get adapters
    LibMudletSessionAdapter* getSessionAdapter() const;
    
    // Migration helpers
    void migrateTriggers();
    void migrateAliases();
    void migrateTimers();
    
    // Process events (call from Host::processEvents)
    void processLibMudletEvents();

private:
    Host* mHost;
    std::unique_ptr<LibMudletQtAdapter> mCoreAdapter;
    std::unique_ptr<LibMudletSessionAdapter> mSessionAdapter;
    std::shared_ptr<mudlet::Session> mSession;
    bool mInitialized = false;
};

/**
 * @brief Compatibility macros for gradual migration
 */
#define LIBMUDLET_AVAILABLE 1

// Conditional compilation helpers
#ifdef MUDLET_USE_LIBMUDLET_TRIGGERS
    #define IF_LIBMUDLET_TRIGGERS(code) code
    #define IF_LEGACY_TRIGGERS(code)
#else
    #define IF_LIBMUDLET_TRIGGERS(code)
    #define IF_LEGACY_TRIGGERS(code) code
#endif

#ifdef MUDLET_USE_LIBMUDLET_ALIASES
    #define IF_LIBMUDLET_ALIASES(code) code
    #define IF_LEGACY_ALIASES(code)
#else
    #define IF_LIBMUDLET_ALIASES(code)
    #define IF_LEGACY_ALIASES(code) code
#endif

#ifdef MUDLET_USE_LIBMUDLET_TIMERS
    #define IF_LIBMUDLET_TIMERS(code) code
    #define IF_LEGACY_TIMERS(code)
#else
    #define IF_LIBMUDLET_TIMERS(code)
    #define IF_LEGACY_TIMERS(code) code
#endif

#endif // MUDLET_LIBMUDLET_INTEGRATION_H