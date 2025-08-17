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

#include "trigger_engine_impl.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>

namespace mudlet {

TriggerEngineImpl::TriggerEngineImpl() 
    : mPatternMatcher(PatternMatcher::create()) {
}

TriggerEngineImpl::~TriggerEngineImpl() = default;

int TriggerEngineImpl::addTrigger(const Trigger& trigger) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    int id = mNextTriggerId++;
    auto triggerData = std::make_unique<TriggerData>();
    triggerData->trigger = trigger;
    triggerData->needsRecompilation = true;
    
    if (!compileTrigger(*triggerData)) {
        Logger::warning("Failed to compile trigger pattern: {}", trigger.pattern);
        return -1;
    }
    
    mTriggers[id] = std::move(triggerData);
    Logger::debug("Added trigger {} with pattern: {}", id, trigger.pattern);
    
    return id;
}

bool TriggerEngineImpl::removeTrigger(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTriggers.find(id);
    if (it == mTriggers.end()) {
        return false;
    }
    
    mTriggers.erase(it);
    Logger::debug("Removed trigger {}", id);
    return true;
}

const Trigger* TriggerEngineImpl::getTrigger(int id) const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTriggers.find(id);
    if (it == mTriggers.end()) {
        return nullptr;
    }
    
    return &(it->second->trigger);
}

bool TriggerEngineImpl::updateTrigger(int id, const Trigger& trigger) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTriggers.find(id);
    if (it == mTriggers.end()) {
        return false;
    }
    
    it->second->trigger = trigger;
    it->second->needsRecompilation = true;
    
    if (!compileTrigger(*it->second)) {
        Logger::warning("Failed to recompile updated trigger {}: {}", id, trigger.pattern);
        return false;
    }
    
    Logger::debug("Updated trigger {}", id);
    return true;
}

std::vector<Trigger> TriggerEngineImpl::getAllTriggers() const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    std::vector<Trigger> triggers;
    triggers.reserve(mTriggers.size());
    
    for (const auto& [id, triggerData] : mTriggers) {
        triggers.push_back(triggerData->trigger);
    }
    
    return triggers;
}

std::vector<int> TriggerEngineImpl::processLine(const TextLine& line) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    if (!mTriggersEnabled) {
        return {};
    }
    
    std::vector<int> firedTriggers;
    
    // Process triggers in priority order
    auto triggerIds = getSortedTriggerIds();
    
    for (int id : triggerIds) {
        auto it = mTriggers.find(id);
        if (it == mTriggers.end()) {
            continue;
        }
        
        auto& triggerData = *it->second;
        
        if (!shouldProcessTrigger(triggerData)) {
            continue;
        }
        
        // Recompile if needed
        if (triggerData.needsRecompilation) {
            if (!compileTrigger(triggerData)) {
                continue;
            }
        }
        
        // Match against the pattern
        if (triggerData.compiledPattern) {
            auto result = triggerData.compiledPattern->match(line.text);
            if (result.matched) {
                firedTriggers.push_back(id);
                
                // Update trigger statistics
                updateTriggerStats(triggerData);
                
                // Call the trigger callback
                if (mTriggerCallback) {
                    MatchInfo matchInfo = convertMatchResult(result);
                    matchInfo.line = line;
                    mTriggerCallback(id, matchInfo);
                }
                
                // Check if this is a one-shot trigger
                if (triggerData.trigger.fireLimit > 0) {
                    triggerData.trigger.fireCount++;
                    if (triggerData.trigger.fireCount >= triggerData.trigger.fireLimit) {
                        triggerData.trigger.enabled = false;
                    }
                }
                
                // Break if this trigger should stop further processing
                if (triggerData.trigger.stopProcessing) {
                    break;
                }
            }
        }
    }
    
    return firedTriggers;
}

bool TriggerEngineImpl::setTriggerEnabled(int id, bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTriggers.find(id);
    if (it == mTriggers.end()) {
        return false;
    }
    
    it->second->trigger.enabled = enabled;
    Logger::debug("Trigger {} {}", id, enabled ? "enabled" : "disabled");
    return true;
}

void TriggerEngineImpl::setAllTriggersEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    mTriggersEnabled = enabled;
    Logger::debug("All triggers {}", enabled ? "enabled" : "disabled");
}

void TriggerEngineImpl::clearTriggers() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    mTriggers.clear();
    Logger::debug("Cleared all triggers");
}

void TriggerEngineImpl::setTriggerCallback(std::function<void(int triggerId, const MatchInfo& match)> callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mTriggerCallback = callback;
}

bool TriggerEngineImpl::compileTrigger(TriggerData& triggerData) {
    if (!mPatternMatcher) {
        return false;
    }
    
    triggerData.compiledPattern = mPatternMatcher->compilePattern(
        triggerData.trigger.pattern,
        triggerData.trigger.patternType,
        triggerData.trigger.caseSensitive
    );
    
    triggerData.needsRecompilation = false;
    
    if (!triggerData.compiledPattern) {
        Logger::error("Failed to compile trigger pattern '{}': {}", 
                     triggerData.trigger.pattern, 
                     mPatternMatcher->getLastError());
        return false;
    }
    
    return true;
}

MatchInfo TriggerEngineImpl::convertMatchResult(const PatternMatcher::MatchResult& result) {
    MatchInfo info;
    info.matched = result.matched;
    info.fullMatch = result.fullMatch;
    info.captures = result.captures;
    info.startPos = result.startPos;
    info.endPos = result.endPos;
    info.namedGroups = result.namedGroups;
    return info;
}

std::vector<int> TriggerEngineImpl::getSortedTriggerIds() const {
    std::vector<int> ids;
    ids.reserve(mTriggers.size());
    
    for (const auto& [id, triggerData] : mTriggers) {
        ids.push_back(id);
    }
    
    // Sort by priority (higher priority first), then by ID for stable ordering
    std::sort(ids.begin(), ids.end(), [this](int a, int b) {
        auto itA = mTriggers.find(a);
        auto itB = mTriggers.find(b);
        
        if (itA == mTriggers.end() || itB == mTriggers.end()) {
            return a < b;
        }
        
        const auto& triggerA = itA->second->trigger;
        const auto& triggerB = itB->second->trigger;
        
        if (triggerA.priority != triggerB.priority) {
            return triggerA.priority > triggerB.priority;
        }
        
        return a < b;
    });
    
    return ids;
}

bool TriggerEngineImpl::shouldProcessTrigger(const TriggerData& triggerData) const {
    const auto& trigger = triggerData.trigger;
    
    // Check if trigger is enabled
    if (!trigger.enabled) {
        return false;
    }
    
    // Check fire limit
    if (trigger.fireLimit > 0 && trigger.fireCount >= trigger.fireLimit) {
        return false;
    }
    
    // Check cooldown
    if (trigger.cooldownMs > 0) {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastFire = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - trigger.lastFireTime
        ).count();
        
        if (timeSinceLastFire < trigger.cooldownMs) {
            return false;
        }
    }
    
    return true;
}

void TriggerEngineImpl::updateTriggerStats(TriggerData& triggerData) {
    auto& trigger = triggerData.trigger;
    trigger.fireCount++;
    trigger.lastFireTime = std::chrono::steady_clock::now();
}

} // namespace mudlet
