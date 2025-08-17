#ifndef LIBMUDLET_TRIGGER_ENGINE_IMPL_H
#define LIBMUDLET_TRIGGER_ENGINE_IMPL_H

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
 * @brief Trigger engine implementation extracted from TTrigger logic
 */
class TriggerEngineImpl : public TriggerEngine {
public:
    TriggerEngineImpl();
    ~TriggerEngineImpl() override;
    
    // TriggerEngine interface implementation
    int addTrigger(const Trigger& trigger) override;
    bool removeTrigger(int id) override;
    const Trigger* getTrigger(int id) const override;
    bool updateTrigger(int id, const Trigger& trigger) override;
    std::vector<Trigger> getAllTriggers() const override;
    
    std::vector<int> processLine(const TextLine& line) override;
    
    bool setTriggerEnabled(int id, bool enabled) override;
    void setAllTriggersEnabled(bool enabled) override;
    void clearTriggers() override;
    
    void setTriggerCallback(std::function<void(int triggerId, const MatchInfo& match)> callback) override;

private:
    struct TriggerData {
        Trigger trigger;
        std::unique_ptr<PatternMatcher::CompiledPattern> compiledPattern;
        bool needsRecompilation = false;
    };
    
    mutable std::mutex mMutex;
    std::unordered_map<int, std::unique_ptr<TriggerData>> mTriggers;
    std::unique_ptr<PatternMatcher> mPatternMatcher;
    std::function<void(int, const MatchInfo&)> mTriggerCallback;
    
    int mNextTriggerId = 1;
    bool mTriggersEnabled = true;
    
    // Internal methods
    bool compileTrigger(TriggerData& triggerData);
    MatchInfo convertMatchResult(const PatternMatcher::MatchResult& result);
    std::vector<int> getSortedTriggerIds() const;
    bool shouldProcessTrigger(const TriggerData& triggerData) const;
    void updateTriggerStats(TriggerData& triggerData);
};

} // namespace mudlet

#endif // LIBMUDLET_TRIGGER_ENGINE_IMPL_H