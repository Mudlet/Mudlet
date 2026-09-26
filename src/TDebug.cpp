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

#include "Host.h"
#include "TBuffer.h"

#include <QTime>

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

/* static */ void TDebug::announceFilters()
{
    if (!smpSink) {
        return;
    }

    const int hidden = hiddenCategoryCount();
    if (hidden) {
        //: Shown in the Central Debug Console when it opens with some kinds of message hidden. %n is how many.
        smpSink->printDebugLine(csmTagSystemMessage % tr("%n kind(s) of message are hidden - use the controls below to change that.\n", "", hidden), Qt::white, Qt::darkBlue, QString());
    }

    if (!smItemFilter.isEmpty()) {
        //: Shown in the Central Debug Console when it opens narrowed to a single trigger, alias, timer and so on. %1 is that item's name.
        smpSink->printDebugLine(csmTagSystemMessage % tr("Showing only messages about \"%1\" - use the controls below to change that.\n").arg(smItemFilter), Qt::white, Qt::darkBlue, QString());
    }
}

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

/* static */ void TDebug::discardPausedMessages()
{
    smPausedQueue.clear();
    smPausedDroppedCount = 0;
}

// Keeps what it couldn't print: the console going away is no reason to lose the user's messages.
/* static */ void TDebug::drainPausedQueue()
{
    if (!smpSink) {
        return;
    }

    if (smPausedDroppedCount) {
        // Ahead of the replay: the cap drops the OLDEST messages, so the gap is at the top:
        //: Shown in the Central Debug Console on resuming, when more messages arrived while paused than could be held back.
        smpSink->printDebugLine(csmTagSystemMessage % tr("%n message(s) dropped while paused.\n", "", smPausedDroppedCount), Qt::white, Qt::darkRed, QString());
        smPausedDroppedCount = 0;
    }

    while (!smPausedQueue.isEmpty()) {
        if (!smpSink) {
            return;
        }
        const auto message = smPausedQueue.dequeue();
        // Already composed when it arrived, profile marking and all:
        smpSink->printDebugLine(message.mMessage, message.mForeground, message.mBackground, message.mTimeStamp);
    }
}

// Empty for the dummy message used to flush the queue.
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

// Filtering here, not when drawing, means toggling a filter never disturbs what is on screen and
// filtered-out messages cost nothing.
bool TDebug::passesFilters(const Host* pHost)
{
    if (msg.isEmpty() && !pHost) {
        // The dummy message used to flush the queue when the console is first
        // shown - it must never be filtered out:
        return true;
    }

    if (msg.startsWith(csmContinue)) {
        // A continuation shares its head's fate, or the console is left with orphaned fragments:
        if (smLastMessagePassed) {
            return true;
        }
        // ...unless only the text filter held the head: the searched-for text often lives in the fragment
        // (the trigger name after "ERROR:", the game line after "new line arrived:"), so a match brings it back:
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
    // Case-insensitive to agree with the completer that offered the name:
    if (!smItemFilter.isEmpty() && mItemName.compare(smItemFilter, Qt::CaseInsensitive) != 0 && mCategory != Category::System) {
        // System messages still pass, so the console doesn't look dead. Must stay AHEAD of the text filter:
        // the held-head rule re-admits a head on a text match alone, which is safe only because it passed here:
        return false;
    }
    if (!smTextFilter.isEmpty() && !msg.contains(smTextFilter, smTextFilterCaseSensitivity)) {
        smHeadHeld = true;
        return false;
    }

    smLastMessagePassed = true;
    return true;
}

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

    if (Q_UNLIKELY(!smpSink)) {
        if (Q_LIKELY(!line.isEmpty())) {
            // Don't enqueue empty messages
            // Stamped here rather than when the sink turns up, so that a
            // backlog replayed minutes later still reads as when it happened:
            smMessageQueue.enqueue(TDebugMessage(line, QString(), foreground, background, QTime::currentTime().toString(TBuffer::smTimeStampFormat)));
        }
        return;
    }

    if (Q_UNLIKELY(!smMessageQueue.isEmpty())) {
        // The sink must have just come on-line - so unload all the messages
        // stacked up while there was none:
        while (!smMessageQueue.isEmpty() && smpSink) {
            const auto message = smMessageQueue.dequeue();
            smpSink->printDebugLine(message.mMessage, message.mForeground, message.mBackground, message.mTimeStamp);
        }
    }

    if (line.isEmpty()) {
        // The dummy message used to flush the backlog above
        return;
    }

    if (smpSink) {
        smpSink->printDebugLine(line, foreground, background, QString());
    }
}

// This is the method that pushes the accumulated text out to the Central Debug
// Console, after the filters have had their say.
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
        if (smpProfileObserver) {
            smpProfileObserver->profileRenamed(newName, pair.second);
            smpProfileObserver->profilesChanged();
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
    if (smpProfileObserver) {
        smpProfileObserver->profilesChanged();
    }
    TDebug localMessage(Qt::blue, Qt::white, Category::System);
    localMessage << qsl("Profile '%1' started.\n").arg(hostName) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black, Category::System);
    tableMessage << TDebug::displayNewTable() >> nullptr;
    if (smDebugMode && smpProfileObserver) {
        smpProfileObserver->profileAddedInDebugMode();
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

    // Forget the filter setting of inactive profiles, or a later Host at the same address would be
    // silenced. Pruning against the whole map also covers the name lookup above finding nothing:
    QSet<const Host*> stillActive;
    for (auto it = smIdentifierMap.cbegin(); it != smIdentifierMap.cend(); ++it) {
        stillActive.insert(it.key());
    }
    smDisabledHosts.intersect(stillActive);
    if (smpProfileObserver) {
        smpProfileObserver->profilesChanged();
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

    // Left untranslated, like most Central Debug Console content
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
