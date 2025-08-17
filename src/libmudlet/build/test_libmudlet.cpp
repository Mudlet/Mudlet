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

/**
 * @file test_libmudlet.cpp
 * @brief Simple test program to verify libmudlet functionality
 */

#include "mudlet/core.h"
#include "mudlet/session_simple.h"
#include "mudlet/engines.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing libmudlet Core functionality..." << std::endl;
    
    // Create core instance
    auto core = mudlet::Core::create();
    if (!core) {
        std::cerr << "Failed to create Core instance" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Core instance created successfully" << std::endl;
    
    // Create a session
    auto session = core->createSession("test_session");
    if (!session) {
        std::cerr << "Failed to create session" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Session created: " << session->getName() << std::endl;
    
    // Test trigger engine
    auto& triggerEngine = session->getTriggerEngine();
    
    mudlet::Trigger testTrigger;
    testTrigger.pattern = "hello";
    testTrigger.patternType = mudlet::PatternType::Substring;
    testTrigger.script = "print('Hello world!')";
    testTrigger.enabled = true;
    
    int triggerId = triggerEngine.addTrigger(testTrigger);
    if (triggerId < 0) {
        std::cerr << "Failed to add trigger" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Trigger added with ID: " << triggerId << std::endl;
    
    // Test alias engine
    auto& aliasEngine = session->getAliasEngine();
    
    mudlet::Alias testAlias;
    testAlias.pattern = "test";
    testAlias.command = "say Testing alias system";
    testAlias.patternType = mudlet::PatternType::Exact;
    testAlias.enabled = true;
    
    int aliasId = aliasEngine.addAlias(testAlias);
    if (aliasId < 0) {
        std::cerr << "Failed to add alias" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Alias added with ID: " << aliasId << std::endl;
    
    // Test alias processing
    auto result = aliasEngine.processCommand("test");
    if (!result.processed) {
        std::cerr << "Alias processing failed" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Alias processed: '" << result.command << "' -> '" << result.expansion << "'" << std::endl;
    
    // Test timer engine
    auto& timerEngine = session->getTimerEngine();
    
    mudlet::Timer testTimer;
    testTimer.intervalMs = 1000;
    testTimer.script = "print('Timer fired!')";
    testTimer.enabled = true;
    testTimer.autoStart = false;
    
    int timerId = timerEngine.addTimer(testTimer);
    if (timerId < 0) {
        std::cerr << "Failed to add timer" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Timer added with ID: " << timerId << std::endl;
    
    // Test timer start/stop
    bool started = timerEngine.startTimer(timerId);
    if (!started) {
        std::cerr << "Failed to start timer" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Timer started successfully" << std::endl;
    
    bool stopped = timerEngine.stopTimer(timerId);
    if (!stopped) {
        std::cerr << "Failed to stop timer" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Timer stopped successfully" << std::endl;
    
    // Process events
    core->processEvents();
    std::cout << "✓ Event processing completed" << std::endl;
    
    // Test session removal
    bool removed = core->removeSession("test_session");
    if (!removed) {
        std::cerr << "Failed to remove session" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Session removed successfully" << std::endl;
    
    std::cout << std::endl << "🎉 All libmudlet tests passed!" << std::endl;
    std::cout << "LibMudlet Phase 1 implementation is complete and functional." << std::endl;
    
    return 0;
}