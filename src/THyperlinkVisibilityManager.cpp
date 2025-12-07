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

THyperlinkVisibilityManager::THyperlinkVisibilityManager(TConsole* pConsole, QObject* parent)
: QObject(parent)
, mpConsole(pConsole)
{
    mpTimer = new QTimer(this);
    mpTimer->setInterval(100); // Check every 100ms for timer-based concealments
    connect(mpTimer, &QTimer::timeout, this, &THyperlinkVisibilityManager::slot_checkTimers);
}

THyperlinkVisibilityManager::~THyperlinkVisibilityManager()
{
    if (mpTimer) {
        mpTimer->stop();
    }
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
    default:
        tracked.action = TrackedHyperlink::Action::None;
        break;
    }
    tracked.delayMs = styling.visibility.delayMs;
    tracked.onPrompt = styling.visibility.onPrompt;
    tracked.deletesEntireLine = styling.visibility.deletesEntireLine;
    tracked.isConcealed = styling.visibility.isConcealed;

    mTrackedLinks.insert(linkId, tracked);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug().noquote() << "[OSC8-Visibility] Registered hyperlink" << linkId 
                       << "at line" << lineNumber << "col" << startColumn
                       << "length" << length
                       << "action:" << (tracked.action == TrackedHyperlink::Action::Conceal ? "conceal" : "reveal")
                       << "delayMs:" << tracked.delayMs
                       << "onPrompt:" << tracked.onPrompt
                       << "deletesEntireLine:" << tracked.deletesEntireLine
                       << "isConcealed:" << tracked.isConcealed;
#endif

    if (tracked.delayMs > 0) {
        mHasTimerBasedLinks = true;
        startTimerIfNeeded();
    }
    
    // Return true if this link should start concealed (caller should replace text with spaces)
    return tracked.isConcealed;
}

void THyperlinkVisibilityManager::onLinkClicked(int linkId)
{
    if (!mTrackedLinks.contains(linkId)) {
        return;
    }

    TrackedHyperlink& link = mTrackedLinks[linkId];
    
    // For conceal actions with a delay, clicking activates the timer
    if (link.action == TrackedHyperlink::Action::Conceal && link.delayMs > 0 && link.timerActivatedMs == 0) {
        link.timerActivatedMs = QDateTime::currentMSecsSinceEpoch();
#if defined(DEBUG_OSC_PROCESSING)
        qDebug().noquote() << "[OSC8-Visibility] Link" << linkId << "clicked - timer activated, will conceal in" << link.delayMs << "ms";
#endif
        mHasTimerBasedLinks = true;
        startTimerIfNeeded();
    }
}

void THyperlinkVisibilityManager::onCommandLineTextChanged()
{
    processPromptTriggeredLinks();
}

void THyperlinkVisibilityManager::onCommandLineSubmitted()
{
    processPromptTriggeredLinks();
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
}

void THyperlinkVisibilityManager::processPromptTriggeredLinks()
{
    bool changed = false;
    
    for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
        TrackedHyperlink& link = it.value();
        
        if (!link.onPrompt) {
            continue;
        }

        if (link.action == TrackedHyperlink::Action::Conceal && !link.isConcealed) {
            performConcealment(link);
            changed = true;
        } else if (link.action == TrackedHyperlink::Action::Reveal && link.isConcealed) {
            performReveal(link);
            changed = true;
        }
    }
    
    if (changed) {
        emit visibilityChanged();
    }
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
    stopTimerIfNotNeeded();;
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
            
            performConcealment(link);
            changed = true;
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
            
            performReveal(link);
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
        if (link.lineNumber >= 0 && link.lineNumber < buffer.size()) {
            buffer.deleteLine(link.lineNumber);
            
            for (auto it = mTrackedLinks.begin(); it != mTrackedLinks.end(); ++it) {
                if (it.key() != link.linkId && it.value().lineNumber > link.lineNumber) {
                    it.value().lineNumber--;
                }
            }
        }
    } else {
        if (link.lineNumber >= 0 && link.lineNumber < buffer.lineBuffer.size()) {
            QString& lineText = buffer.lineBuffer[link.lineNumber];
            
            if (link.startColumn >= 0 && link.startColumn + link.length <= lineText.length()) {
                QString spaces(link.length, ' ');
                lineText.replace(link.startColumn, link.length, spaces);
                buffer.clearLinkIndices(link.lineNumber, link.startColumn, link.length);
            }
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
