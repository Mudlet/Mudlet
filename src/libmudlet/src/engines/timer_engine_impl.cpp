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

#include "timer_engine_impl.h"
#include "utils/logger.h"
#include <algorithm>

namespace mudlet {

TimerEngineImpl::TimerEngineImpl() = default;

TimerEngineImpl::~TimerEngineImpl() = default;

int TimerEngineImpl::addTimer(const Timer& timer) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    int id = mNextTimerId++;
    auto timerData = std::make_unique<TimerData>();
    timerData->timer = timer;
    timerData->active = false;
    
    mTimers[id] = std::move(timerData);
    Logger::debug("Added timer {} with interval: {}ms", id, timer.intervalMs);
    
    // Auto-start if requested
    if (timer.autoStart) {
        startTimer(id);
    }
    
    return id;
}

bool TimerEngineImpl::removeTimer(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return false;
    }
    
    mTimers.erase(it);
    Logger::debug("Removed timer {}", id);
    return true;
}

const Timer* TimerEngineImpl::getTimer(int id) const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return nullptr;
    }
    
    return &(it->second->timer);
}

bool TimerEngineImpl::updateTimer(int id, const Timer& timer) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return false;
    }
    
    bool wasActive = it->second->active;
    it->second->timer = timer;
    
    // Restart if it was active
    if (wasActive) {
        calculateNextFireTime(*it->second);
    }
    
    Logger::debug("Updated timer {}", id);
    return true;
}

std::vector<Timer> TimerEngineImpl::getAllTimers() const {
    std::lock_guard<std::mutex> lock(mMutex);
    
    std::vector<Timer> timers;
    timers.reserve(mTimers.size());
    
    for (const auto& [id, timerData] : mTimers) {
        timers.push_back(timerData->timer);
    }
    
    return timers;
}

bool TimerEngineImpl::startTimer(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return false;
    }
    
    auto& timerData = *it->second;
    timerData.active = true;
    timerData.startTime = std::chrono::steady_clock::now();
    calculateNextFireTime(timerData);
    
    Logger::debug("Started timer {}", id);
    return true;
}

bool TimerEngineImpl::stopTimer(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return false;
    }
    
    it->second->active = false;
    Logger::debug("Stopped timer {}", id);
    return true;
}

bool TimerEngineImpl::resetTimer(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return false;
    }
    
    auto& timerData = *it->second;
    timerData.timer.fireCount = 0;
    
    if (timerData.active) {
        timerData.startTime = std::chrono::steady_clock::now();
        calculateNextFireTime(timerData);
    }
    
    Logger::debug("Reset timer {}", id);
    return true;
}

std::vector<int> TimerEngineImpl::processTimers() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    if (!mTimersEnabled) {
        return {};
    }
    
    std::vector<int> firedTimers;
    auto now = std::chrono::steady_clock::now();
    
    auto activeIds = getActiveTimerIds();
    
    for (int id : activeIds) {
        auto it = mTimers.find(id);
        if (it == mTimers.end()) {
            continue;
        }
        
        auto& timerData = *it->second;
        
        if (!shouldProcessTimer(timerData)) {
            continue;
        }
        
        // Check if timer should fire
        if (now >= timerData.nextFireTime) {
            firedTimers.push_back(id);
            
            // Update timer statistics
            updateTimerStats(timerData);
            
            // Call the timer callback
            if (mTimerCallback) {
                mTimerCallback(id);
            }
            
            // Check if this is a limited timer
            if (timerData.timer.fireLimit > 0 && 
                timerData.timer.fireCount >= timerData.timer.fireLimit) {
                timerData.active = false;
                Logger::debug("Timer {} reached fire limit and stopped", id);
            } else {
                // Calculate next fire time for recurring timers
                calculateNextFireTime(timerData);
            }
        }
    }
    
    return firedTimers;
}

bool TimerEngineImpl::setTimerEnabled(int id, bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        return false;
    }
    
    it->second->timer.enabled = enabled;
    Logger::debug("Timer {} {}", id, enabled ? "enabled" : "disabled");
    return true;
}

void TimerEngineImpl::setAllTimersEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    mTimersEnabled = enabled;
    Logger::debug("All timers {}", enabled ? "enabled" : "disabled");
}

void TimerEngineImpl::clearTimers() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    mTimers.clear();
    Logger::debug("Cleared all timers");
}

void TimerEngineImpl::setTimerCallback(std::function<void(int timerId)> callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mTimerCallback = callback;
}

void TimerEngineImpl::calculateNextFireTime(TimerData& timerData) {
    auto interval = std::chrono::milliseconds(timerData.timer.intervalMs);
    
    if (timerData.timer.fireCount == 0) {
        // First fire - use initial delay if specified, otherwise use interval
        auto delay = timerData.timer.initialDelayMs > 0 ? 
                    std::chrono::milliseconds(timerData.timer.initialDelayMs) : 
                    interval;
        timerData.nextFireTime = timerData.startTime + delay;
    } else {
        // Subsequent fires - add interval to last fire time
        timerData.nextFireTime += interval;
    }
}

bool TimerEngineImpl::shouldProcessTimer(const TimerData& timerData) const {
    const auto& timer = timerData.timer;
    
    // Check if timer is enabled and active
    if (!timer.enabled || !timerData.active) {
        return false;
    }
    
    // Check fire limit
    if (timer.fireLimit > 0 && timer.fireCount >= timer.fireLimit) {
        return false;
    }
    
    return true;
}

void TimerEngineImpl::updateTimerStats(TimerData& timerData) {
    timerData.timer.fireCount++;
}

std::vector<int> TimerEngineImpl::getActiveTimerIds() const {
    std::vector<int> ids;
    
    for (const auto& [id, timerData] : mTimers) {
        if (timerData->active) {
            ids.push_back(id);
        }
    }
    
    return ids;
}

} // namespace mudlet
