/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "THyperlinkVisibilityManager.h"
#include "TBuffer.h"
#include "TConsole.h"
#include "TTextEdit.h"

#include <QDateTime>
#include <QDebug>
#include <limits>

THyperlinkVisibilityManager::THyperlinkVisibilityManager(TConsole* pConsole, QObject* parent)
: QObject(parent)
, mpConsole(pConsole)
{
    mpTimer = new QTimer(this);
    mpTimer->setInterval(100); // Check every 100ms for timer-based concealments
    connect(mpTimer, &QTimer::timeout, this, &THyperlinkVisibilityManager::slot_checkTimers);

    mpOutputGapTimer = new QTimer(this);
    mpOutputGapTimer->setSingleShot(true);
    connect(mpOutputGapTimer, &QTimer::timeout, this, &THyperlinkVisibilityManager::slot_outputGapExpired);
}

THyperlinkVisibilityManager::~THyperlinkVisibilityManager()
{
    mpTimer->stop();
    mpOutputGapTimer->stop();
}

bool THyperlinkVisibilityManager::registerHyperlink(int linkId, int lineNumber, int startColumn, int length,
                                                    const QString& originalText, const Mudlet::HyperlinkStyling& styling)
{
    if (!styling.visibility.hasVisibilitySettings) {
        return false;
    }

    TrackedHyperlink tracked;
    tracked.linkId = linkId;
    tracked.lineNumber = lineNumber;
    tracked.startColumn = startColumn;
    tracked.length = length;
    tracked.originalText = originalText;
    tracked.creationTimeMs = QDateTime::currentMSecsSinceEpoch();
    
    switch (styling.visibility.action) {
    case Mudlet::HyperlinkStyling::VisibilitySettings::Action::Conceal:
        tracked.action = TrackedHyperlink::Action::Conceal;
        break;
    case Mudlet::HyperlinkStyling::VisibilitySettings::Action::Reveal:
        tracked.action = TrackedHyperlink::Action::Reveal;
        break;
    case Mudlet::HyperlinkStyling::VisibilitySettings::Action::RevealThenConceal:
        tracked.action = TrackedHyperlink::Action::RevealThenConceal;
        tracked.phase = TrackedHyperlink::Phase::Initial;  // Start in initial phase, waiting for reveal
        break;
    default:
        tracked.action = TrackedHyperlink::Action::None;
        break;
    }
    tracked.delayMs = styling.visibility.delayMs;
    tracked.deletesEntireLine = styling.visibility.deletesEntireLine;
    tracked.isConcealed = styling.visibility.isConcealed;
    
    // Copy expire trigger settings
    tracked.expireOnInput = styling.visibility.expireOnInput;
    tracked.expireOnPrompt = styling.visibility.expireOnPrompt;
    tracked.expireOnOutput = styling.visibility.expireOnOutput;
    tracked.outputDelayMs = styling.visibility.outputDelayMs;
    // Skip the first prompt since it likely fires immediately after the link is printed
    tracked.skipFirstPrompt = styling.visibility.expireOnPrompt;
    // Skip the first output gap since the timer starts immediately during link registration
    tracked.skipFirstOutput = styling.visibility.expireOnOutput;

    mTrackedLinks.insert(linkId, tracked);

#if defined(DEBUG_OSC_PROCESSING)
    QString actionStr;
    switch (tracked.action) {
    case TrackedHyperlink::Action::Conceal:
        actionStr = qsl("conceal");
        break;
    case TrackedHyperlink::Action::Reveal:
        actionStr = qsl("reveal");
        break;
    case TrackedHyperlink::Action::RevealThenConceal:
        actionStr = qsl("reveal+conceal");
        break;
    default:
        actionStr = qsl("none");
        break;
    }
    qDebug().noquote() << "[OSC8-Visibility] Registered hyperlink" << linkId 
                       << "at line" << lineNumber << "col" << startColumn
                       << "length" << length
                       << "action:" << actionStr
                       << "delayMs:" << tracked.delayMs
                       << "deletesEntireLine:" << tracked.deletesEntireLine
                       << "isConcealed:" << tracked.isConcealed
                       << "expireOnInput:" << tracked.expireOnInput
                       << "expireOnPrompt:" << tracked.expireOnPrompt
                       << "expireOnOutput:" << tracked.expireOnOutput
                       << "outputDelayMs:" << tracked.outputDelayMs;
#endif

    // Handle reveal with zero delay - reveal immediately (don't start concealed)
    if (tracked.action == TrackedHyperlink::Action::Reveal && tracked.delayMs == 0 
        && !tracked.expireOnInput && !tracked.expireOnPrompt && !tracked.expireOnOutput) {
#if defined(DEBUG_OSC_PROCESSING)
        qDebug().noquote() << "[OSC8-Visibility] Reveal link" << linkId << "has zero delay and no expire triggers - will be visible immediately";
#endif
        // Don't start concealed - return false so text is NOT replaced with spaces
        mTrackedLinks[linkId].isConcealed = false;
        return false;
    }

    // Handle RevealThenConceal with zero delay - start in Revealed phase so clicking works
    if (tracked.action == TrackedHyperlink::Action::RevealThenConceal && tracked.delayMs == 0) {
#if defined(DEBUG_OSC_PROCESSING)
        qDebug().noquote() << "[OSC8-Visibility] RevealThenConceal link" << linkId << "has zero delay - starting in Revealed phase";
#endif
        mTrackedLinks[linkId].phase = TrackedHyperlink::Phase::Revealed;
        mTrackedLinks[linkId].isConcealed = false;
        // Still need timer active for the conceal-after-click behavior
        mHasTimerBasedLinks = true;
        startTimerIfNeeded();
        return false;
    }

    if (tracked.delayMs > 0) {
        mHasTimerBasedLinks = true;
        startTimerIfNeeded();
    }
    
    // Return true if this link should start concealed (caller should replace text with spaces)
    qWarning() << "[REVEAL-DEBUG] registerHyperlink returning isConcealed=" << tracked.isConcealed << "for linkId" << linkId;
    return tracked.isConcealed;
}

void THyperlinkVisibilityManager::onLinkClicked(int linkId)
{
#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] onLinkClicked called with linkId:" << linkId 
                       << "tracked links count:" << mTrackedLinks.size()
                       << "contains linkId:" << mTrackedLinks.contains(linkId);
#endif
    if (!mTrackedLinks.contains(linkId)) {
        return;
    }

    TrackedHyperlink& link = mTrackedLinks[linkId];
    
    // For conceal actions, clicking activates the concealment
    if (link.action == TrackedHyperlink::Action::Conceal && !link.isConcealed) {
        const bool hasExpireTriggers = link.expireOnInput || link.expireOnPrompt || link.expireOnOutput;
        
        if (link.delayMs == 0 && !hasExpireTriggers) {
            // Immediate concealment (no delay and no expire triggers)
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Link" << linkId << "clicked - concealing immediately (no delay, no expire triggers)";
#endif
            performConcealment(link);
            emit visibilityChanged();
        } else if (link.delayMs > 0 && link.timerActivatedMs == 0) {
            // Delayed concealment - start timer
            link.timerActivatedMs = QDateTime::currentMSecsSinceEpoch();
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Link" << linkId << "clicked - timer activated, will conceal in" << link.delayMs << "ms";
#endif
            mHasTimerBasedLinks = true;
            startTimerIfNeeded();
        }
    }
    
    // For RevealThenConceal links that have been revealed, clicking starts the conceal phase
    if (link.action == TrackedHyperlink::Action::RevealThenConceal 
        && link.phase == TrackedHyperlink::Phase::Revealed) {
        
        if (link.delayMs == 0) {
            // Immediate concealment after click
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] RevealThenConceal link" << linkId << "clicked - concealing immediately";
#endif
            link.phase = TrackedHyperlink::Phase::Concealed;
            performConcealment(link);
            emit visibilityChanged();
        } else {
            // Delayed concealment - start timer
            link.phase = TrackedHyperlink::Phase::WaitingToConceal;
            link.timerActivatedMs = QDateTime::currentMSecsSinceEpoch();
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] RevealThenConceal link" << linkId << "clicked - will conceal in" << link.delayMs << "ms";
#endif
            mHasTimerBasedLinks = true;
            startTimerIfNeeded();
        }
    }
}

void THyperlinkVisibilityManager::unregisterHyperlink(int linkId)
{
    if (mTrackedLinks.remove(linkId) > 0) {
#if defined(DEBUG_OSC_PROCESSING)
        qDebug().noquote() << "[OSC8-Visibility] Unregistered hyperlink" << linkId << "(link removed from buffer)";
#endif
        // If this was the last link with timer-based actions, update the flag
        bool hasTimers = false;
        for (const auto& link : mTrackedLinks) {
            if (link.delayMs > 0 || link.expireOnOutput) {
                hasTimers = true;
                break;
            }
        }
        if (!hasTimers && mHasTimerBasedLinks) {
            mHasTimerBasedLinks = false;
            mpTimer->stop();
        }
    }
}

void THyperlinkVisibilityManager::onUserInput()
{
#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] onUserInput called";
#endif
    processExpireTriggeredLinks(true, false, false);
}

void THyperlinkVisibilityManager::onPromptReceived()
{
#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] onPromptReceived called (GA/EOR)";
#endif
    processExpireTriggeredLinks(false, true, false);
}

void THyperlinkVisibilityManager::onDataReceived()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Check if any tracked links have output expiry enabled
    quint32 minOutputDelay = 0;
    bool hasOutputLinks = false;
    
    for (const auto& link : mTrackedLinks) {
        if (link.expireOnOutput) {
            hasOutputLinks = true;
            if (minOutputDelay == 0 || link.outputDelayMs < minOutputDelay) {
                minOutputDelay = link.outputDelayMs;
            }
        }
    }
    
    if (!hasOutputLinks) {
        mLastDataReceivedMs = currentTime;
        return;
    }
    
    // Sammer's "batch" approach: expire links when new data arrives AFTER a gap
    // This creates the "one batch at a time" effect - old links are hidden when
    // a new batch of output begins, not during the idle period
    if (mLastDataReceivedMs > 0 && mpOutputGapTimer && !mpOutputGapTimer->isActive()) {
        // Gap timer has expired (not active) - this is new data after an idle gap
        // Trigger output expiry now, at the start of the new batch
#if defined(DEBUG_OSC_PROCESSING)
        qDebug().noquote() << "[OSC8-Visibility] New data after gap - triggering output expire (batch transition)";
#endif
        processExpireTriggeredLinks(false, false, true);
    }
    
    mLastDataReceivedMs = currentTime;
    
    // Start/restart the output gap timer with the minimum delay
    // Clamp to INT_MAX to prevent truncation when casting to int for QTimer::start()
    if (mpOutputGapTimer && minOutputDelay > 0) {
        const quint32 clampedDelay = std::min(minOutputDelay, static_cast<quint32>(std::numeric_limits<int>::max()));
        mpOutputGapTimer->start(static_cast<int>(clampedDelay));
    }
}

void THyperlinkVisibilityManager::slot_outputGapExpired()
{
    // Gap timer expired - we are now "in gap state"
    // The actual expiry will happen when new data arrives (in onDataReceived)
    // This matches Sammer's "batch" approach: expire old links at the START of
    // a new batch, not during the idle period
#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] Output gap timer expired - waiting for new data to trigger batch transition";
#endif
}

void THyperlinkVisibilityManager::processExpireTriggeredLinks(bool input, bool prompt, bool output)
{
    bool changed = false;
    
    for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
        TrackedHyperlink& link = it.value();
        
        // Handle skipFirstPrompt - if this is a prompt trigger and the link should skip the first one
        if (prompt && link.skipFirstPrompt) {
            link.skipFirstPrompt = false;
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Link" << link.linkId << "skipping first prompt trigger";
#endif
            continue;
        }
        
        // Handle skipFirstOutput - if this is an output trigger and the link should skip the first one
        if (output && link.skipFirstOutput) {
            link.skipFirstOutput = false;
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Link" << link.linkId << "skipping first output trigger";
#endif
            continue;
        }
        
        // Check if this trigger applies to this link
        bool shouldTrigger = (input && link.expireOnInput) ||
                            (prompt && link.expireOnPrompt) ||
                            (output && link.expireOnOutput);
        
        if (!shouldTrigger) {
            continue;
        }

        if (link.action == TrackedHyperlink::Action::Conceal && !link.isConcealed) {
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Expire trigger concealing link" << link.linkId
                               << "input:" << input << "prompt:" << prompt << "output:" << output;
#endif
            performConcealment(link);
            changed = true;
        } else if (link.action == TrackedHyperlink::Action::Reveal && link.isConcealed) {
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Expire trigger revealing link" << link.linkId
                               << "input:" << input << "prompt:" << prompt << "output:" << output;
#endif
            performReveal(link);
            changed = true;
        } else if (link.action == TrackedHyperlink::Action::RevealThenConceal) {
            // Handle RevealThenConceal based on current phase
            if (link.phase == TrackedHyperlink::Phase::Initial && link.isConcealed) {
                // Phase 1: Reveal the link
#if defined(DEBUG_OSC_PROCESSING)
                qDebug().noquote() << "[OSC8-Visibility] RevealThenConceal link" << link.linkId 
                                   << "- expire trigger revealing (phase: Initial -> Revealed)";
#endif
                performReveal(link);
                link.phase = TrackedHyperlink::Phase::Revealed;
                changed = true;
            }
            // Note: Conceal phase for RevealThenConceal is triggered by click, not by expire triggers
        }
    }
    
    if (changed) {
        emit visibilityChanged();
    }
}

void THyperlinkVisibilityManager::concealLink(int linkId)
{
    if (!mTrackedLinks.contains(linkId)) {
        return;
    }

    TrackedHyperlink& link = mTrackedLinks[linkId];

    if (link.isConcealed) {
        return;
    }

    performConcealment(link);
    emit visibilityChanged();
}

void THyperlinkVisibilityManager::revealLink(int linkId)
{
    if (!mTrackedLinks.contains(linkId)) {
        return;
    }

    TrackedHyperlink& link = mTrackedLinks[linkId];

    if (!link.isConcealed) {
        return;
    }

    performReveal(link);
    emit visibilityChanged();
}

bool THyperlinkVisibilityManager::isLinkConcealed(int linkId) const
{
    if (!mTrackedLinks.contains(linkId)) {
        return false;
    }
    return mTrackedLinks.value(linkId).isConcealed;
}

void THyperlinkVisibilityManager::removeLinksOnLine(int lineNumber)
{
    QList<int> toRemove;
    
    for (auto it = mTrackedLinks.constBegin(); it != mTrackedLinks.constEnd(); ++it) {
        if (it.value().lineNumber == lineNumber) {
            toRemove.append(it.key());
        }
    }
    
    for (int linkId : toRemove) {
        mTrackedLinks.remove(linkId);
    }
    
    stopTimerIfNotNeeded();
}

void THyperlinkVisibilityManager::adjustLineNumbers(int deletedLineStart, int deletedLineCount)
{
    QList<int> toRemove;
    
    for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
        TrackedHyperlink& link = it.value();
        
        if (link.lineNumber >= deletedLineStart && link.lineNumber < deletedLineStart + deletedLineCount) {
            toRemove.append(it.key());
        } else if (link.lineNumber >= deletedLineStart + deletedLineCount) {
            link.lineNumber -= deletedLineCount;
        }
    }
    
    for (int linkId : toRemove) {
        mTrackedLinks.remove(linkId);
    }
    
    stopTimerIfNotNeeded();
}

void THyperlinkVisibilityManager::clear()
{
    mTrackedLinks.clear();
    mHasTimerBasedLinks = false;
    mLastDataReceivedMs = 0;
    if (mpOutputGapTimer) {
        mpOutputGapTimer->stop();
    }
    stopTimerIfNotNeeded();
}

void THyperlinkVisibilityManager::slot_checkTimers()
{
    if (mTrackedLinks.isEmpty()) {
        stopTimerIfNotNeeded();
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    bool changed = false;
    bool stillHasTimerLinks = false;
    
    // Collect link IDs that need action to avoid iterator invalidation
    QVector<int> linksToReveal;
    QVector<int> linksToConceal;
    QVector<int> linksToTransition;  // For RevealThenConceal phase transitions

    for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
        TrackedHyperlink& link = it.value();

        if (link.delayMs == 0) {
            continue;
        }

        // For conceal actions on visible links, timer starts when clicked
        if (link.action == TrackedHyperlink::Action::Conceal && !link.isConcealed) {
            // Timer hasn't been activated by click yet - keep waiting
            if (link.timerActivatedMs == 0) {
                stillHasTimerLinks = true;
                continue;
            }
            
            qint64 elapsed = currentTime - link.timerActivatedMs;
            if (elapsed < link.delayMs) {
                stillHasTimerLinks = true;
                continue;
            }
            
            linksToConceal.append(it.key());
        } else if (link.action == TrackedHyperlink::Action::Reveal && link.isConcealed) {
            // For reveal actions, timer starts immediately from creation
            qint64 elapsed = currentTime - link.creationTimeMs;
#if defined(DEBUG_OSC_PROCESSING)
            qDebug().noquote() << "[OSC8-Visibility] Checking reveal link" << it.key() 
                               << "elapsed:" << elapsed << "delayMs:" << link.delayMs;
#endif
            if (elapsed < link.delayMs) {
                stillHasTimerLinks = true;
                continue;
            }
            
            linksToReveal.append(it.key());
        } else if (link.action == TrackedHyperlink::Action::RevealThenConceal) {
            // Handle RevealThenConceal based on phase
            if (link.phase == TrackedHyperlink::Phase::Initial && link.isConcealed) {
                // Waiting to reveal - timer starts from creation
                qint64 elapsed = currentTime - link.creationTimeMs;
                if (elapsed < link.delayMs) {
                    stillHasTimerLinks = true;
                    continue;
                }
#if defined(DEBUG_OSC_PROCESSING)
                qDebug().noquote() << "[OSC8-Visibility] RevealThenConceal link" << it.key() 
                                   << "- timer revealing (phase: Initial -> Revealed)";
#endif
                linksToTransition.append(it.key());
            } else if (link.phase == TrackedHyperlink::Phase::WaitingToConceal) {
                // Waiting to conceal after click - timer starts from click
                if (link.timerActivatedMs == 0) {
                    stillHasTimerLinks = true;
                    continue;
                }
                qint64 elapsed = currentTime - link.timerActivatedMs;
                if (elapsed < link.delayMs) {
                    stillHasTimerLinks = true;
                    continue;
                }
#if defined(DEBUG_OSC_PROCESSING)
                qDebug().noquote() << "[OSC8-Visibility] RevealThenConceal link" << it.key() 
                                   << "- timer concealing (phase: WaitingToConceal -> Concealed)";
#endif
                linksToConceal.append(it.key());
            } else if (link.phase == TrackedHyperlink::Phase::Revealed) {
                // Revealed and waiting for click - keep timer active
                stillHasTimerLinks = true;
            }
        }
    }
    
    // Now process the collected links (safe because we're not iterating)
    for (int linkId : linksToReveal) {
        if (mTrackedLinks.contains(linkId)) {
            performReveal(mTrackedLinks[linkId]);
            changed = true;
        }
    }
    
    for (int linkId : linksToConceal) {
        if (mTrackedLinks.contains(linkId)) {
            // For RevealThenConceal, mark phase as Concealed
            if (mTrackedLinks[linkId].action == TrackedHyperlink::Action::RevealThenConceal) {
                mTrackedLinks[linkId].phase = TrackedHyperlink::Phase::Concealed;
            }
            performConcealment(mTrackedLinks[linkId]);
            changed = true;
        }
    }
    
    for (int linkId : linksToTransition) {
        if (mTrackedLinks.contains(linkId)) {
            performReveal(mTrackedLinks[linkId]);
            mTrackedLinks[linkId].phase = TrackedHyperlink::Phase::Revealed;
            changed = true;
        }
    }

    mHasTimerBasedLinks = stillHasTimerLinks;
    
    if (!mHasTimerBasedLinks) {
        stopTimerIfNotNeeded();
    }

    if (changed) {
        emit visibilityChanged();
    }
}

void THyperlinkVisibilityManager::startTimerIfNeeded()
{
    if (mpTimer && !mpTimer->isActive() && mHasTimerBasedLinks) {
        mpTimer->start();
    }
}

void THyperlinkVisibilityManager::stopTimerIfNotNeeded()
{
    if (mpTimer && mpTimer->isActive()) {
        bool hasTimerLinks = false;
        for (const auto& link : mTrackedLinks) {
            if (link.delayMs == 0) {
                continue;
            }

            if (link.action == TrackedHyperlink::Action::Conceal && !link.isConcealed) {
                hasTimerLinks = true;
                break;
            }
            if (link.action == TrackedHyperlink::Action::Reveal && link.isConcealed) {
                hasTimerLinks = true;
                break;
            }
            if (link.action == TrackedHyperlink::Action::RevealThenConceal) {
                // Timer needed if in Initial (waiting to reveal), Revealed (waiting for click), 
                // or WaitingToConceal (waiting to conceal after click)
                if (link.phase != TrackedHyperlink::Phase::Concealed) {
                    hasTimerLinks = true;
                    break;
                }
            }
        }
        
        if (!hasTimerLinks) {
            mpTimer->stop();
            mHasTimerBasedLinks = false;
        }
    }
}

void THyperlinkVisibilityManager::performConcealment(TrackedHyperlink& link)
{
    if (!mpConsole) {
        return;
    }

#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] Concealing link" << link.linkId
                       << "deletesEntireLine:" << link.deletesEntireLine;
#endif

    TBuffer& buffer = mpConsole->buffer;

    if (link.deletesEntireLine) {
        // CRITICAL BUG FIX: Prevent cascade deletion by unregistering ALL links on the target line
        // before deleting it. This prevents other links from being adjusted to the wrong line
        // and accidentally triggering on content they shouldn't affect.
        
        const int targetLine = link.lineNumber;
        if (targetLine >= 0 && targetLine < buffer.lineBuffer.size()) {
            // First, collect all link IDs that are on the same line as the one being deleted
            QList<int> linksOnTargetLine;
            for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
                if (it.value().lineNumber == targetLine) {
                    linksOnTargetLine.append(it.key());
                }
            }
            
            // Unregister all links on the target line to prevent interference
            for (int linkId : linksOnTargetLine) {
                if (linkId != link.linkId) { // Don't remove the current link yet
                    mTrackedLinks.remove(linkId);
#if defined(DEBUG_OSC_PROCESSING)
                    qDebug().noquote() << "[OSC8-Visibility] Pre-emptively unregistered co-located link" << linkId;
#endif
                }
            }
            
            // Now delete the line
            buffer.deleteLine(targetLine);
            
            // Remove the current link (it's now invalid)
            mTrackedLinks.remove(link.linkId);
            
            // SAFE line number adjustment: Only adjust links that are on lines > targetLine
            // and do this atomically to prevent cascading issues
            QMap<int, TrackedHyperlink> adjustedLinks;
            for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
                TrackedHyperlink adjustedLink = it.value();
                if (adjustedLink.lineNumber > targetLine) {
                    adjustedLink.lineNumber--;
#if defined(DEBUG_OSC_PROCESSING)
                    qDebug().noquote() << "[OSC8-Visibility] Adjusted link" << it.key() 
                                       << "from line" << (adjustedLink.lineNumber + 1) 
                                       << "to line" << adjustedLink.lineNumber;
#endif
                }
                adjustedLinks.insert(it.key(), adjustedLink);
            }
            
            // Replace the tracked links with the adjusted versions
            mTrackedLinks = adjustedLinks;
            
            // Update display
            if (mpConsole->mUpperPane) {
                mpConsole->mUpperPane->update();
            }
            if (mpConsole->mLowerPane) {
                mpConsole->mLowerPane->update();
            }
            
            // Stop timer if no more timer-based links exist
            bool hasTimers = false;
            for (const auto& remainingLink : mTrackedLinks) {
                if (remainingLink.delayMs > 0 || remainingLink.expireOnOutput) {
                    hasTimers = true;
                    break;
                }
            }
            if (!hasTimers && mHasTimerBasedLinks) {
                mHasTimerBasedLinks = false;
                mpTimer->stop();
            }
            
            return; // Early return - all cleanup done
        }
    } else {
        // Non-destructive concealment (replace text with spaces)
        if (link.lineNumber >= 0 && link.lineNumber < buffer.lineBuffer.size()) {
            QString& lineText = buffer.lineBuffer[link.lineNumber];
            
            if (link.startColumn >= 0 && link.startColumn + link.length <= lineText.length()) {
                QString spaces(link.length, ' ');
                lineText.replace(link.startColumn, link.length, spaces);
                buffer.clearLinkIndices(link.lineNumber, link.startColumn, link.length);
            }
        }

        link.isConcealed = true;

        if (mpConsole->mUpperPane) {
            mpConsole->mUpperPane->update();
        }
        if (mpConsole->mLowerPane) {
            mpConsole->mLowerPane->update();
        }
    }
}

void THyperlinkVisibilityManager::performReveal(TrackedHyperlink& link)
{
    if (!mpConsole) {
        return;
    }

#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] Revealing link" << link.linkId;
#endif

    if (link.deletesEntireLine) {
        qWarning() << "[OSC8-Visibility] Cannot reveal link" << link.linkId << "- line was permanently deleted";
        return;
    }

    TBuffer& buffer = mpConsole->buffer;

    if (link.lineNumber >= 0 && link.lineNumber < buffer.lineBuffer.size()) {
        QString& lineText = buffer.lineBuffer[link.lineNumber];
        
        if (link.startColumn >= 0 && link.startColumn + link.length <= lineText.length()) {
            lineText.replace(link.startColumn, link.length, link.originalText);
            // Restore the link indices so the text is clickable again
            buffer.restoreLinkIndices(link.lineNumber, link.startColumn, link.length, link.linkId);
        }
    }

    link.isConcealed = false;

    if (mpConsole->mUpperPane) {
        mpConsole->mUpperPane->update();
    }
    if (mpConsole->mLowerPane) {
        mpConsole->mLowerPane->update();
    }
}
