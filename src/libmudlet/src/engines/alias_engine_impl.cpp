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

#include "alias_engine_impl.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <regex>

namespace mudlet {

AliasEngineImpl::AliasEngineImpl() 
    : mPatternMatcher(PatternMatcher::create()) {
}

AliasEngineImpl::~AliasEngineImpl() = default;

int AliasEngineImpl::addAlias(const Alias& alias) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    int id = mNextAliasId++;
    auto aliasData = std::make_unique<AliasData>();
    aliasData->alias = alias;
    aliasData->needsRecompilation = true;
    
    if (!compileAlias(*aliasData)) {
        Logger::warning("Failed to compile alias pattern: {}", alias.pattern);
        return -1;
    }
    
    mAliases[id] = std::move(aliasData);
    Logger::debug("Added alias {} with pattern: {}", id, alias.pattern);
    
    return id;
}

bool AliasEngineImpl::removeAlias(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mAliases.find(id);
    if (it == mAliases.end()) {
        return false;
    }
    
    mAliases.erase(it);
    Logger::debug("Removed alias {}", id);
    return true;
}

const Alias* AliasEngineImpl::getAlias(int id) const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mAliases.find(id);
    if (it == mAliases.end()) {
        return nullptr;
    }
    
    return &(it->second->alias);
}

bool AliasEngineImpl::updateAlias(int id, const Alias& alias) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mAliases.find(id);
    if (it == mAliases.end()) {
        return false;
    }
    
    it->second->alias = alias;
    it->second->needsRecompilation = true;
    
    if (!compileAlias(*it->second)) {
        Logger::warning("Failed to recompile updated alias {}: {}", id, alias.pattern);
        return false;
    }
    
    Logger::debug("Updated alias {}", id);
    return true;
}

std::vector<Alias> AliasEngineImpl::getAllAliases() const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    std::vector<Alias> aliases;
    aliases.reserve(mAliases.size());
    
    for (const auto& [id, aliasData] : mAliases) {
        aliases.push_back(aliasData->alias);
    }
    
    return aliases;
}

ProcessResult AliasEngineImpl::processCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    ProcessResult result;
    result.command = command;
    result.processed = false;
    
    if (!mAliasesEnabled || command.empty()) {
        return result;
    }
    
    // Process aliases in priority order
    auto aliasIds = getSortedAliasIds();
    
    for (int id : aliasIds) {
        auto it = mAliases.find(id);
        if (it == mAliases.end()) {
            continue;
        }
        
        auto& aliasData = *it->second;
        
        if (!shouldProcessAlias(aliasData)) {
            continue;
        }
        
        // Recompile if needed
        if (aliasData.needsRecompilation) {
            if (!compileAlias(aliasData)) {
                continue;
            }
        }
        
        // Match against the pattern
        if (aliasData.compiledPattern) {
            auto matchResult = aliasData.compiledPattern->match(command);
            if (matchResult.matched) {
                // Update alias statistics
                updateAliasStats(aliasData);
                
                // Convert match result
                MatchInfo matchInfo = convertMatchResult(matchResult);
                
                // Expand the alias
                std::string expansion = expandAlias(aliasData.alias, matchInfo);
                
                // Set result
                result.processed = true;
                result.expansion = expansion;
                result.aliasId = id;
                result.matchInfo = matchInfo;
                
                // Call the alias callback
                if (mAliasCallback) {
                    mAliasCallback(id, matchInfo, expansion);
                }
                
                // Update fire count for limited aliases
                if (aliasData.alias.fireLimit > 0) {
                    aliasData.alias.fireCount++;
                    if (aliasData.alias.fireCount >= aliasData.alias.fireLimit) {
                        aliasData.alias.enabled = false;
                    }
                }
                
                // First match wins for aliases
                break;
            }
        }
    }
    
    return result;
}

bool AliasEngineImpl::setAliasEnabled(int id, bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mAliases.find(id);
    if (it == mAliases.end()) {
        return false;
    }
    
    it->second->alias.enabled = enabled;
    Logger::debug("Alias {} {}", id, enabled ? "enabled" : "disabled");
    return true;
}

void AliasEngineImpl::setAllAliasesEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    mAliasesEnabled = enabled;
    Logger::debug("All aliases {}", enabled ? "enabled" : "disabled");
}

void AliasEngineImpl::clearAliases() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    mAliases.clear();
    Logger::debug("Cleared all aliases");
}

void AliasEngineImpl::setAliasCallback(std::function<void(int aliasId, const MatchInfo& match, const std::string& expansion)> callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mAliasCallback = callback;
}

bool AliasEngineImpl::compileAlias(AliasData& aliasData) {
    if (!mPatternMatcher) {
        return false;
    }
    
    aliasData.compiledPattern = mPatternMatcher->compilePattern(
        aliasData.alias.pattern,
        aliasData.alias.patternType,
        aliasData.alias.caseSensitive
    );
    
    aliasData.needsRecompilation = false;
    
    if (!aliasData.compiledPattern) {
        Logger::error("Failed to compile alias pattern '{}': {}", 
                     aliasData.alias.pattern, 
                     mPatternMatcher->getLastError());
        return false;
    }
    
    return true;
}

MatchInfo AliasEngineImpl::convertMatchResult(const PatternMatcher::MatchResult& result) {
    MatchInfo info;
    info.matched = result.matched;
    info.fullMatch = result.fullMatch;
    info.captures = result.captures;
    info.startPos = result.startPos;
    info.endPos = result.endPos;
    info.namedGroups = result.namedGroups;
    return info;
}

std::vector<int> AliasEngineImpl::getSortedAliasIds() const {
    std::vector<int> ids;
    ids.reserve(mAliases.size());
    
    for (const auto& [id, aliasData] : mAliases) {
        ids.push_back(id);
    }
    
    // Sort by priority (higher priority first), then by ID for stable ordering
    std::sort(ids.begin(), ids.end(), [this](int a, int b) {
        auto itA = mAliases.find(a);
        auto itB = mAliases.find(b);
        
        if (itA == mAliases.end() || itB == mAliases.end()) {
            return a < b;
        }
        
        const auto& aliasA = itA->second->alias;
        const auto& aliasB = itB->second->alias;
        
        if (aliasA.priority != aliasB.priority) {
            return aliasA.priority > aliasB.priority;
        }
        
        return a < b;
    });
    
    return ids;
}

bool AliasEngineImpl::shouldProcessAlias(const AliasData& aliasData) const {
    const auto& alias = aliasData.alias;
    
    // Check if alias is enabled
    if (!alias.enabled) {
        return false;
    }
    
    // Check fire limit
    if (alias.fireLimit > 0 && alias.fireCount >= alias.fireLimit) {
        return false;
    }
    
    // Check cooldown
    if (alias.cooldownMs > 0) {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastFire = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - alias.lastFireTime
        ).count();
        
        if (timeSinceLastFire < alias.cooldownMs) {
            return false;
        }
    }
    
    return true;
}

void AliasEngineImpl::updateAliasStats(AliasData& aliasData) {
    auto& alias = aliasData.alias;
    alias.fireCount++;
    alias.lastFireTime = std::chrono::steady_clock::now();
}

std::string AliasEngineImpl::expandAlias(const Alias& alias, const MatchInfo& match) const {
    if (alias.command.empty()) {
        return "";
    }
    
    // Substitute captures in the command
    return substituteCaptures(alias.command, match);
}

std::string AliasEngineImpl::substituteCaptures(const std::string& text, const MatchInfo& match) const {
    std::string result = text;
    
    // Replace numbered captures (%1, %2, etc.)
    for (size_t i = 0; i < match.captures.size(); ++i) {
        std::string placeholder = "%" + std::to_string(i + 1);
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), match.captures[i]);
            pos += match.captures[i].length();
        }
    }
    
    // Replace %0 with full match
    size_t pos = 0;
    while ((pos = result.find("%0", pos)) != std::string::npos) {
        result.replace(pos, 2, match.fullMatch);
        pos += match.fullMatch.length();
    }
    
    // Replace named captures (%{name})
    for (const auto& [name, value] : match.namedGroups) {
        std::string placeholder = "%{" + name + "}";
        pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    
    return result;
}

} // namespace mudlet