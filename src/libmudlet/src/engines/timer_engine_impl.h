#ifndef LIBMUDLET_TIMER_ENGINE_IMPL_H
#define LIBMUDLET_TIMER_ENGINE_IMPL_H

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
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>

namespace mudlet {

/**
 * @brief Timer engine implementation extracted from TTimer logic
 */
class TimerEngineImpl : public TimerEngine {
public:
    TimerEngineImpl();
    ~TimerEngineImpl() override;
    
    // TimerEngine interface implementation
    int addTimer(const Timer& timer) override;
    bool removeTimer(int id) override;
    const Timer* getTimer(int id) const override;
    bool updateTimer(int id, const Timer& timer) override;
    std::vector<Timer> getAllTimers() const override;
    
    bool startTimer(int id) override;
    bool stopTimer(int id) override;
    bool resetTimer(int id) override;
    
    std::vector<int> processTimers() override;
    
    bool setTimerEnabled(int id, bool enabled) override;
    void setAllTimersEnabled(bool enabled) override;
    void clearTimers() override;
    
    void setTimerCallback(std::function<void(int timerId)> callback) override;

private:
    struct TimerData {
        Timer timer;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point nextFireTime;
        bool active = false;
    };
    
    mutable std::mutex mMutex;
    std::unordered_map<int, std::unique_ptr<TimerData>> mTimers;
    std::function<void(int)> mTimerCallback;
    
    int mNextTimerId = 1;
    bool mTimersEnabled = true;
    
    // Internal methods
    void calculateNextFireTime(TimerData& timerData);
    bool shouldProcessTimer(const TimerData& timerData) const;
    void updateTimerStats(TimerData& timerData);
    std::vector<int> getActiveTimerIds() const;
};

} // namespace mudlet

#endif // LIBMUDLET_TIMER_ENGINE_IMPL_H