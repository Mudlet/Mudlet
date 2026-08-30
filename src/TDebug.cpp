/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2021-2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@gmail.com          *
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


#include "TDebug.h"

#include "TConsole.h"
#include "TDebugFilterBar.h"
#include "TTabBar.h"
#include "mudlet.h"

#include <chrono>

using namespace std::chrono_literals;

/* static */ const TDebug::Categories TDebug::csmNoisyCategories = {Category::GameLine, Category::TriggerDetail, Category::LuaSuccess, Category::Selection};

/* static */ const TDebug::Categories TDebug::csmAllCategories = {Category::System,
                                                                  Category::Error,
                                                                  Category::Network,
                                                                  Category::Protocol,
                                                                  Category::GameLine,
                                                                  Category::TriggerMatch,
                                                                  Category::TriggerDetail,
                                                                  Category::Alias,
                                                                  Category::Item,
                                                                  Category::LuaSuccess,
                                                                  Category::LuaWarning,
                                                                  Category::Selection,
                                                                  Category::Map,
                                                                  Category::Other};

// The console is unusable with the noisy categories on, so they start off - see
// TDebug::csmNoisyCategories:
/* static */ TDebug::Categories TDebug::smEnabledCategories = TDebug::csmAllCategories & ~TDebug::csmNoisyCategories;

TDebug::TDebug(const QColor& c, const QColor& d, const Category category, const QString& itemName)
: fgColor(c)
, bgColor(d)
, mCategory(category)
, mItemName(itemName)
{
}

/* static */ bool TDebug::wants(const Category category)
{
    return smDebugMode && smEnabledCategories.testFlag(category);
}

/* static */ void TDebug::setEnabledCategories(const Categories categories)
{
    smEnabledCategories = categories;
}

/* static */ void TDebug::setCategoryEnabled(const Category category, const bool enabled)
{
    smEnabledCategories.setFlag(category, enabled);
}

/* static */ void TDebug::setHostEnabled(const Host* pHost, const bool enabled)
{
    if (enabled) {
        smDisabledHosts.remove(pHost);
    } else {
        smDisabledHosts.insert(pHost);
    }
}

/* static */ QList<QPair<const Host*, QString>> TDebug::activeProfiles()
{
    QList<QPair<const Host*, QString>> profiles;
    QMapIterator<const Host*, QPair<QString, QString>> itIdentifier(smIdentifierMap);
    while (itIdentifier.hasNext()) {
        itIdentifier.next();
        if (itIdentifier.key()) {
            profiles.append(qMakePair(itIdentifier.key(), qsl("%1%2").arg(itIdentifier.value().second, itIdentifier.value().first)));
        }
    }
    return profiles;
}

/* static */ void TDebug::setTextFilter(const QString& text, const Qt::CaseSensitivity caseSensitivity)
{
    smTextFilter = text;
    smTextFilterCaseSensitivity = caseSensitivity;
}

/* static */ void TDebug::setPaused(const bool paused)
{
    if (smPaused == paused) {
        return;
    }
    smPaused = paused;
    if (!smPaused) {
        drainPausedQueue();
    }
}

// A console filtered down to nothing looks exactly like a console that has
// stopped working, and some categories are hidden out of the box - so say so
// rather than leaving people to wonder.
/* static */ void TDebug::announceFilters()
{
    QPointer<TConsole> debugConsole = mudlet::smpDebugConsole;
    if (!debugConsole) {
        return;
    }

    const int hidden = hiddenCategoryCount();
    if (hidden) {
        //: Shown in the Central Debug Console when it opens with some kinds of message hidden. %n is how many.
        debugConsole->print(csmTagSystemMessage % tr("%n kind(s) of message are hidden - use the controls below to change that.\n", "", hidden), Qt::white, Qt::darkBlue);
    }

    if (!smItemFilter.isEmpty()) {
        // Much more drastic than hiding a category - everything that is not
        // about this one item is gone, so it needs saying out loud:
        //: Shown in the Central Debug Console when it opens narrowed to a single trigger, alias, timer and so on. %1 is that item's name.
        debugConsole->print(csmTagSystemMessage % tr("Showing only messages about \"%1\" - use the controls below to change that.\n").arg(smItemFilter), Qt::white, Qt::darkBlue);
    }
}

// How many of the categories are currently switched off. Shared so that the
// notice above and the toolbar's own label can never disagree.
/* static */ int TDebug::hiddenCategoryCount()
{
    const Categories hidden = csmAllCategories & ~smEnabledCategories;
    int count = 0;
    for (quint32 bit = 1; bit; bit <<= 1) {
        if (hidden.testFlag(static_cast<Category>(bit))) {
            ++count;
        }
    }
    return count;
}

// Throws away anything held back while paused - used when the user clears the
// console, so that resuming does not immediately refill it.
/* static */ void TDebug::discardPausedMessages()
{
    smPausedQueue.clear();
    smPausedDroppedCount = 0;
}

// Replays everything held back while paused, in the order it arrived. Keeps
// anything it could not print - the console going away is not a reason to throw
// the user's messages out.
/* static */ void TDebug::drainPausedQueue()
{
    QPointer<TConsole> debugConsole = mudlet::smpDebugConsole;
    if (!debugConsole) {
        return;
    }

    if (smPausedDroppedCount) {
        // Ahead of the replay, because it is the OLDEST messages that the cap
        // had to throw away - the gap is at the top, not the bottom:
        //: Shown in the Central Debug Console on resuming, when more messages arrived while paused than could be held back.
        debugConsole->print(csmTagSystemMessage % tr("%n message(s) dropped while paused.\n", "", smPausedDroppedCount), Qt::white, Qt::darkRed);
        smPausedDroppedCount = 0;
    }

    while (!smPausedQueue.isEmpty()) {
        debugConsole = mudlet::smpDebugConsole;
        if (!debugConsole) {
            // Console has gone - leave the remainder queued
            return;
        }
        const auto message = smPausedQueue.dequeue();
        // Already composed when it arrived, profile marking and all:
        debugConsole->print(message.mMessage, message.mForeground, message.mBackground, message.mTimeStamp);
    }
}

// The text to print for this message, profile marking included. Returns an
// empty string for the dummy message used to flush the queue.
QString TDebug::displayLine(Host* pHost)
{
    if (pHost && !smIdentifierMap.contains(pHost)) {
        // A Host we have no record of is one being destroyed, so treat this as
        // a system message rather than registering it as a new profile:
        if (msg.startsWith(csmContinue)) {
            msg.remove(0, 1);
        }
        return csmTagSystemMessage % msg;
    }
    return composeLine(deduceProfileTag(msg, pHost), msg);
}

// The profile marking is only worth showing when it tells the reader something
// they cannot already infer:
/* static */ QString TDebug::composeLine(const QString& profileTag, const QString& text)
{
    if (profileTag.isNull()) {
        return text;
    }
    if (profileTag == csmTagSystemMessage || Q_UNLIKELY(profileTag == csmTagFault) || smIdentifierMap.count() > 1) {
        return profileTag % text;
    }
    // Only one profile active - so don't print the tag:
    return text;
}

// Decides whether this message should reach the console at all. Filtering here,
// rather than when drawing, means enabling or disabling a filter never disturbs
// what is already on screen and that filtered-out messages cost nothing.
bool TDebug::passesFilters(const Host* pHost)
{
    if (msg.isEmpty() && !pHost) {
        // The dummy message used to flush the queue when the console is first
        // shown - it must never be filtered out:
        return true;
    }

    if (msg.startsWith(csmContinue)) {
        // A continuation of the preceding message: it has to share that
        // message's fate or the console is left with orphaned fragments such as
        // a bare "<some captured text>":
        if (smLastMessagePassed) {
            return true;
        }
        // ...unless the head was held back by the text filter alone. The text
        // people search for usually lives in the fragment - the trigger name in
        // "ERROR:", the game line in "new line arrived:" - so a fragment that
        // matches brings its head back with it:
        if (smHeadHeld && msg.contains(smTextFilter, smTextFilterCaseSensitivity)) {
            smLastMessagePassed = true;
            return true;
        }
        return false;
    }

    smLastMessagePassed = false;
    smHeadHeld = false;

    if (!smEnabledCategories.testFlag(mCategory)) {
        return false;
    }
    if (pHost && smDisabledHosts.contains(pHost)) {
        return false;
    }
    // Case-insensitively, to agree with the completer that offered the name in
    // the first place - typing "combat" after being shown "Combat trigger"
    // should not silently match nothing:
    if (!smItemFilter.isEmpty() && mItemName.compare(smItemFilter, Qt::CaseInsensitive) != 0 && mCategory != Category::System) {
        // Focusing on one item still leaves profile starts and ends visible,
        // so the console does not look dead when nothing is happening.
        // NOTE: this test must stay AHEAD of the text filter below - the
        // held-head rule lets a fragment re-admit its head on a text match
        // alone, which is only safe because the head already passed here:
        return false;
    }
    if (!smTextFilter.isEmpty() && !msg.contains(smTextFilter, smTextFilterCaseSensitivity)) {
        smHeadHeld = true;
        return false;
    }

    smLastMessagePassed = true;
    return true;
}

// Sends a composed line on its way: into the paused queue, into the backlog
// that builds up before the console exists, or straight onto the console.
/* static */ void TDebug::emitLine(const QString& line, const QColor& foreground, const QColor& background)
{
    if (Q_UNLIKELY(smPaused)) {
        if (line.isEmpty()) {
            return;
        }
        if (smPausedQueue.count() >= csmPausedQueueLimit) {
            smPausedQueue.dequeue();
            ++smPausedDroppedCount;
        }
        smPausedQueue.enqueue(TDebugMessage(line, QString(), foreground, background, QTime::currentTime().toString(TBuffer::smTimeStampFormat)));
        return;
    }

    if (Q_UNLIKELY(!mudlet::smpDebugConsole)) {
        if (Q_LIKELY(!line.isEmpty())) {
            // Don't enqueue empty messages
            smMessageQueue.enqueue(TDebugMessage(line, QString(), foreground, background));
        }
        return;
    }

    if (Q_UNLIKELY(!smMessageQueue.isEmpty())) {
        // The console must have just come on-line - so unload all the messages
        // stacked up while it did not exist:
        while (!smMessageQueue.isEmpty()) {
            QPointer<TConsole> backlogConsole = mudlet::smpDebugConsole;
            if (!backlogConsole) {
                break;
            }
            const auto message = smMessageQueue.dequeue();
            backlogConsole->print(message.mMessage, message.mForeground, message.mBackground);
        }
    }

    if (line.isEmpty()) {
        // The dummy message used to flush the backlog above
        return;
    }

    QPointer<TConsole> debugConsole = mudlet::smpDebugConsole;
    if (debugConsole) {
        debugConsole->print(line, foreground, background);
    }
}

// This is the method that pushes the accumulated text out to the Central Debug
// Console, after the filters have had their say. Handles 'msg' beginning with
// 'csmContinue', otherwise if more than 1 profile active, prepends msg with an
// indicator of the profile from which it came, which is deduced from the
// supplied Host pointer.
TDebug& TDebug::operator>>(Host* pHost)
{
    if (!passesFilters(pHost)) {
        if (smHeadHeld) {
            // Compose it now and keep it, in case a fragment of the same
            // message turns out to match the text filter:
            smHeldHead = displayLine(pHost);
            smHeldHeadForeground = fgColor;
            smHeldHeadBackground = bgColor;
        }
        return *this;
    }

    if (Q_UNLIKELY(smHeadHeld)) {
        // A fragment matched, so its head goes out in front of it:
        smHeadHeld = false;
        emitLine(smHeldHead, smHeldHeadForeground, smHeldHeadBackground);
        smHeldHead.clear();
    }

    emitLine(displayLine(pHost), fgColor, bgColor);
    return *this;
}

TDebug& TDebug::operator<<(const QString& t)
{
    msg += t;
    return *this;
}

TDebug& TDebug::operator<<(const QChar& t)
{
    msg += t;
    return *this;
}

TDebug& TDebug::operator<<(const int& t)
{
    msg += QString::number(t);
    return *this;
}

TDebug& TDebug::operator<<(const QMap<QString, QString>& map)
{
    for (QMap<QString, QString>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += qsl("(%1, %2)").arg(it.key(), it.value());
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QMap<QString, int>& map)
{
    for (QMap<QString, int>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += qsl("(%1, %2)").arg(it.key(), QString::number(it.value()));
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QMap<int, QString>& map)
{
    for (QMap<int, QString>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += qsl("(%1, %2)").arg(QString::number(it.key()), it.value());
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QMap<int, int>& map)
{
    for (QMap<int, int>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += qsl("(%1, %2)").arg(QString::number(it.key()), QString::number(it.value()));
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QList<QString>& list)
{
    for (QList<QString>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it) {
        msg += qsl("%1, ").arg(*it);
    }
    msg += ")";
    return *this;
}

TDebug& TDebug::operator<<(const QList<int>& list)
{
    for (QList<int>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it) {
        msg += qsl("%1, ").arg(QString::number(*it));
    }
    msg += "), ";
    return *this;
}

void TDebug::changeHostName(const Host* pHost, const QString& newName)
{
    if (pHost) {
        QPair<QString, QString>& pair = TDebug::smIdentifierMap[pHost];
        pair.first = newName;
        mudlet::self()->mpTabBar->applyPrefixToDisplayedText(newName, pair.second);
        if (mudlet::smpDebugFilterBar) {
            mudlet::smpDebugFilterBar->refreshProfiles();
        }
    }
}

/* static */ void TDebug::addHost(Host* pHost, const QString hostName)
{
    if (!initialised) {
        smAvailableIdentifiers << qsl("[A] ") << qsl("[B] ") << qsl("[C] ") << qsl("[D] ") << qsl("[E] ") << qsl("[F] ") << qsl("[G] ") << qsl("[H] ") << qsl("[I] ") << qsl("[J] ") << qsl("[K] ")
                               << qsl("[L] ") << qsl("[M] ") << qsl("[N] ") << qsl("[O] ") << qsl("[P] ") << qsl("[Q] ") << qsl("[R] ") << qsl("[S] ") << qsl("[T] ") << qsl("[U] ") << qsl("[V] ")
                               << qsl("[W] ") << qsl("[X] ") << qsl("[Y] ") << qsl("[Z] ");
        initialised = true;
    }

    if (!pHost) {
        return;
    }

    QPair<QString, QString> newIdentifier;
    if (TDebug::smAvailableIdentifiers.isEmpty()) {
        // Run out of identifiers - use fall-back one:
        newIdentifier = qMakePair(hostName, csmTagOverflow);
        TDebug::smIdentifierMap.insert(pHost, newIdentifier);
    } else {
        newIdentifier = qMakePair(hostName, smAvailableIdentifiers.dequeue());
        TDebug::smIdentifierMap.insert(pHost, newIdentifier);
    }
    if (mudlet::smpDebugFilterBar) {
        mudlet::smpDebugFilterBar->refreshProfiles();
    }
    TDebug localMessage(Qt::blue, Qt::white, Category::System);
    localMessage << qsl("Profile '%1' started.\n").arg(hostName) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black, Category::System);
    tableMessage << TDebug::displayNewTable() >> nullptr;
    if (smDebugMode) {
        // Can't use TTabBar::applyPrefixToDisplayedText(hostName, newIdentifier.second)
        // here as the profile's tab has not been added to the tabbar yet.
        // Instead arrange for all the tabs to be refreshed when we are next
        // idle:
        QTimer::singleShot(0ms, mudlet::self(), []() {
            mudlet::self()->refreshTabBar();
        });
    }
}

/* static */ void TDebug::removeHost(Host* pHost, const QString hostName)
{
    QPair<QString, QString> identifier;
    const Host* removedHost = pHost;

    if (pHost) {
        // Normal case: remove by Host pointer
        identifier = TDebug::smIdentifierMap.take(pHost);
    } else {
        // Host is being destroyed: find by hostName and remove
        const Host* foundHost = nullptr;

        for (auto it = smIdentifierMap.begin(); it != smIdentifierMap.end(); ++it) {
            if (it.value().first == hostName) {
                foundHost = it.key();
                identifier = it.value();
                break;
            }
        }

        if (foundHost) {
            smIdentifierMap.remove(foundHost);
            removedHost = foundHost;
        }
    }

    // Check for the use of non-profile specific tags:
    if (identifier.second != csmTagOverflow && identifier.second != csmTagSystemMessage && identifier.second != csmTagFault) {
        // is a normal identifier so push it in at the back of the queue for reuse:
        smAvailableIdentifiers.enqueue(identifier.second);
    }

    // Forget the filter setting of every profile that is no longer active -
    // a stale pointer could otherwise be matched by a later, unrelated Host
    // allocated at the same address, silencing it for no visible reason.
    // Pruning against the whole map rather than removing the one profile
    // covers the case where the name lookup above found nothing:
    QSet<const Host*> stillActive;
    for (auto it = smIdentifierMap.cbegin(); it != smIdentifierMap.cend(); ++it) {
        stillActive.insert(it.key());
    }
    smDisabledHosts.intersect(stillActive);
    if (mudlet::smpDebugFilterBar) {
        mudlet::smpDebugFilterBar->refreshProfiles();
    }

    TDebug localMessage(Qt::darkGray, Qt::white, Category::System);
    localMessage << qsl("Profile '%1' ended.\n").arg(hostName) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black, Category::System);
    tableMessage << TDebug::displayNewTable() >> nullptr;
}

/* static */ QString TDebug::displayNewTable()
{
    if (TDebug::smIdentifierMap.count() <= 1) {
        return QString();
    }

    // The identifier legend is left untranslated - most Central Debug Console
    // content still is, although some of it now is not
    QStringList messageLines;
    QMapIterator<const Host*, QPair<QString, QString>> itIdentifier(TDebug::smIdentifierMap);
    while (itIdentifier.hasNext()) {
        itIdentifier.next();
        if (itIdentifier.key()) {
            // Each identifier includes spaces, so no need to include one before
            // the '=' sign:
            messageLines.append(qsl(" %1= \"%2\"").arg(itIdentifier.value().second, itIdentifier.value().first));
        }
    }
    if (messageLines.count() > 1) {
        std::sort(messageLines.begin(), messageLines.end());
    }
    messageLines.prepend(qsl(" %1= System message, not belonging to a specific profile").arg(csmTagSystemMessage));
    // The line wrapping of these texts is a bit less than one might expect
    // because the default size will clip the text otherwise, unless the
    // user resizes the CDC:
    messageLines.prepend(qsl("%1 profiles active now. Each message from a profile \n"
                             "will be prefixed as follows:")
                                 .arg(TDebug::smIdentifierMap.count()));

    return messageLines.join(QChar::LineFeed).append(QChar::LineFeed);
}

/* static */ void TDebug::flushMessageQueue()
{
    TDebug localMessage(Qt::black, Qt::white, Category::System);
    localMessage << QString() >> nullptr;
}

// This will strip a TDebug::csmContinue QChar if present from the start of text:
/* static */ QString TDebug::deduceProfileTag(QString& text, Host* pHost)
{
    if (text.startsWith(csmContinue)) {
        text.remove(0, 1);
    }
    if (pHost) {
        if (!smIdentifierMap.contains(pHost)) {
            // Oops, we do not have that Host on file, better create something
            // for it - this will also cause a pair of new TDebug messages to
            // be created and processed prior to the call to this method being
            // completed:
            addHost(pHost, pHost->getName());
        }
        // By now smIdentifierMap WILL contain something for pHost - but use an
        // the "fault" mark (a bang/exclaimation point) if something is really
        // screwy and it does not, as it happens we have a method that will do
        // that already:
        return getTag(pHost);
    }
    // Must be a system message - or the dummy one to flush the queue.
    if (!text.isEmpty()) {
        // A system message:
        return csmTagSystemMessage;
    }
    // The dummy one:
    return QString();
}

/* static */ QString TDebug::getTag(Host* pHost)
{
    return smIdentifierMap.value(pHost, qMakePair(QString(), csmTagFault)).second;
}
