#ifndef LIBMUDLET_ALIAS_ENGINE_IMPL_H
#define LIBMUDLET_ALIAS_ENGINE_IMPL_H

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

#include "mudlet/engines.h"
#include "utils/pattern_matcher.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

namespace mudlet {

/**
 * @brief Alias engine implementation extracted from TAlias logic
 */
class AliasEngineImpl : public AliasEngine {
public:
    AliasEngineImpl();
    ~AliasEngineImpl() override;
    
    // AliasEngine interface implementation
    int addAlias(const Alias& alias) override;
    bool removeAlias(int id) override;
    const Alias* getAlias(int id) const override;
    bool updateAlias(int id, const Alias& alias) override;
    std::vector<Alias> getAllAliases() const override;
    
    ProcessResult processCommand(const std::string& command) override;
    
    bool setAliasEnabled(int id, bool enabled) override;
    void setAllAliasesEnabled(bool enabled) override;
    void clearAliases() override;
    
    void setAliasCallback(std::function<void(int aliasId, const MatchInfo& match, const std::string& expansion)> callback) override;

private:
    struct AliasData {
        Alias alias;
        std::unique_ptr<PatternMatcher::CompiledPattern> compiledPattern;
        bool needsRecompilation = false;
    };
    
    mutable std::mutex mMutex;
    std::unordered_map<int, std::unique_ptr<AliasData>> mAliases;
    std::unique_ptr<PatternMatcher> mPatternMatcher;
    std::function<void(int, const MatchInfo&, const std::string&)> mAliasCallback;
    
    int mNextAliasId = 1;
    bool mAliasesEnabled = true;
    
    // Internal methods
    bool compileAlias(AliasData& aliasData);
    MatchInfo convertMatchResult(const PatternMatcher::MatchResult& result);
    std::vector<int> getSortedAliasIds() const;
    bool shouldProcessAlias(const AliasData& aliasData) const;
    void updateAliasStats(AliasData& aliasData);
    std::string expandAlias(const Alias& alias, const MatchInfo& match) const;
    std::string substituteCaptures(const std::string& text, const MatchInfo& match) const;
};

} // namespace mudlet

#endif // LIBMUDLET_ALIAS_ENGINE_IMPL_H