/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2021 by Stephen Lyons - slysven@virginmedia.com   *
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
#include "TTabBar.h"
#include "mudlet.h"

// This is a Unicode NON-character code which is explicitly undisplayable but
// can be embedded for our own internal purposes:
const QChar TDebug::csmContinue = QChar(0xFFFF);

QMap<const Host*, QPair<QString, QString>> TDebug::smIdentifierMap;
QQueue<QString> TDebug::smAvailableIdentifiers;
bool TDebug::initialised = false;
QQueue<TDebugMessage> TDebug::smMessageQueue;

TDebug::TDebug(const QColor& c, const QColor& d)
: fgColor(c)
, bgColor(d)
{
}

// This is the method that pushes the accumulated text out to the Central Debug
// Console. Handles 'msg' beginning with 'csmContinue', otherwise if more than 1
// profile active, prepends msg with an indicator of the profile from which it
// came, which is deduced from the supplied Host pointer.
TDebug& TDebug::operator>>(Host* pHost)
{
    if (Q_UNLIKELY(!mudlet::mpDebugConsole)) {
        if (Q_LIKELY(!msg.isEmpty())) {
            // Don't enqueue empty messages
            auto tag = deduceProfileTag(msg, pHost);
            TDebugMessage newMessage(msg, tag, fgColor, bgColor);
            smMessageQueue.enqueue(newMessage);
        }

    } else {
        if (Q_UNLIKELY(!smMessageQueue.isEmpty())) {
            // The mpDebugConsole must have just come on-line - so unload all
            // the stacked up messages:
            while (!smMessageQueue.isEmpty()) {
                const auto& message = smMessageQueue.dequeue();
                if (message.mTag.isNull()) {
                    mudlet::mpDebugConsole->print(message.mMessage, message.mForeground, message.mBackground);
                } else {
                    mudlet::mpDebugConsole->print(message.mTag % message.mMessage, message.mForeground, message.mBackground);
                }
            }
        }

        auto tag = deduceProfileTag(msg, pHost);
        if (tag.isNull()) {
            // We use an empty message with no host pointer to flush out the
            // enqueued messages the first time the CDC is shown - so in that
            // case we will already done everything needed in previous chunk
            // of code. Otherwise just print the message without a tag marking:
            if (!msg.isEmpty()) {
                mudlet::mpDebugConsole->print(msg, fgColor, bgColor);
            }
        } else if (tag == csmTagSystemMessage || Q_UNLIKELY(tag == csmTagFault) || TDebug::smIdentifierMap.count() > 1) {
            // This is a system message or something went wrong in identifying the profile or more than one profile is active
            mudlet::mpDebugConsole->print(tag % msg, fgColor, bgColor);
        } else {
            // Only one profile active - so don't print the tag:
            mudlet::mpDebugConsole->print(msg, fgColor, bgColor);
        }
    }
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
        msg += QStringLiteral("(%1, %2)").arg(it.key(), it.value());
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QMap<QString, int>& map)
{
    for (QMap<QString, int>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += QStringLiteral("(%1, %2)").arg(it.key(), QString::number(it.value()));
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QMap<int, QString>& map)
{
    for (QMap<int, QString>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += QStringLiteral("(%1, %2)").arg(QString::number(it.key()), it.value());
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QMap<int, int>& map)
{
    for (QMap<int, int>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        msg += QStringLiteral("(%1, %2)").arg(QString::number(it.key()), QString::number(it.value()));
    }
    msg += "), ";
    return *this;
}

TDebug& TDebug::operator<<(const QList<QString>& list)
{
    for (QList<QString>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it) {
        msg += QStringLiteral("%1, ").arg(*it);
    }
    msg += ")";
    return *this;
}

TDebug& TDebug::operator<<(const QList<int>& list)
{
    for (QList<int>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it) {
        msg += QStringLiteral("%1, ").arg(QString::number(*it));
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
    }
}

/* static */ void TDebug::addHost(Host* pHost)
{
    if (!initialised) {
        smAvailableIdentifiers << QStringLiteral("[A] ") << QStringLiteral("[B] ") << QStringLiteral("[C] ") << QStringLiteral("[D] ") << QStringLiteral("[E] ")
                               << QStringLiteral("[F] ") << QStringLiteral("[G] ") << QStringLiteral("[H] ") << QStringLiteral("[I] ") << QStringLiteral("[J] ")
                               << QStringLiteral("[K] ") << QStringLiteral("[L] ") << QStringLiteral("[M] ") << QStringLiteral("[N] ") << QStringLiteral("[O] ")
                               << QStringLiteral("[P] ") << QStringLiteral("[Q] ") << QStringLiteral("[R] ") << QStringLiteral("[S] ") << QStringLiteral("[T] ")
                               << QStringLiteral("[U] ") << QStringLiteral("[V] ") << QStringLiteral("[W] ") << QStringLiteral("[X] ") << QStringLiteral("[Y] ")
                               << QStringLiteral("[Z] ");
        initialised = true;
    }

    if (!pHost) {
        return;
    }

    QString hostName = pHost->getName();
    // Take a deep-copy to prevent RVO of the Host::getName() method from
    // stopping deleting the 'Host::mHostName` when the profile is destroyed
    // - so this copy can persist independently of the original when the latter
    // goes away:
    hostName.detach();
    QPair<QString, QString> newIdentifier;
    if (TDebug::smAvailableIdentifiers.isEmpty()) {
        // Run out of identifers - use fall-back one:
        newIdentifier = qMakePair(hostName, csmTagOverflow);
        TDebug::smIdentifierMap.insert(pHost, newIdentifier);
    } else {
        newIdentifier = qMakePair(hostName, smAvailableIdentifiers.dequeue());
        TDebug::smIdentifierMap.insert(pHost, newIdentifier);
    }
    TDebug localMessage(Qt::blue, Qt::white);
    localMessage << QStringLiteral("Profile '%1' started.\n").arg(hostName) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black);
    tableMessage << TDebug::displayNewTable() >> nullptr;
    if (mudlet::debugMode) {
        // Can't use TTabBar::applyPrefixToDisplayedText(hostName, newIdentifier.second)
        // here as the profile's tab has not been added to the tabbar yet.
        // Instead arrange for all the tabs to be refreshed when we are next
        // idle:
        QTimer::singleShot(0, mudlet::self(), []() {
            mudlet::self()->refreshTabBar();
        });
    }
}

/* static */ void TDebug::removeHost(Host* pHost)
{
    auto identifier = TDebug::smIdentifierMap.take(pHost);
    // Check for the use of non-profile specific tags:
    if (identifier.second != csmTagOverflow && identifier.second != csmTagSystemMessage && identifier.second != csmTagFault) {
        // is a normal identifier so push it in at the back of the queue for reuse:
        smAvailableIdentifiers.enqueue(identifier.second);
    }
    TDebug localMessage(Qt::darkGray, Qt::white);
    localMessage << QStringLiteral("Profile '%1' ended.\n").arg(pHost->getName()) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black);
    tableMessage << TDebug::displayNewTable() >> nullptr;
}

/* static */ QString TDebug::displayNewTable()
{
    if (TDebug::smIdentifierMap.count() <= 1) {
        return QString();
    }

    // We do not translate this currently as the Central Debug Console does not
    // get translated content - yet?
    QStringList messageLines;
    QMapIterator<const Host*, QPair<QString, QString>> itIdentifier(TDebug::smIdentifierMap);
    while (itIdentifier.hasNext()) {
        itIdentifier.next();
        if (itIdentifier.key()) {
            // Each identifier includes spaces, so no need to include one before
            // the '=' sign:
            messageLines.append(QStringLiteral(" %1= \"%2\"").arg(itIdentifier.value().second, itIdentifier.value().first));
        }
    }
    if (messageLines.count() > 1) {
        std::sort(messageLines.begin(), messageLines.end());
    }
    messageLines.prepend(QStringLiteral(" %1= System message, not belonging to a specific profile").arg(csmTagSystemMessage));
    // The line wrapping of these texts is a bit less than one might expect
    // because the default size will clip the text otherwise, unless the
    // user resizes the CDC:
    messageLines.prepend(QStringLiteral("%1 profiles active now. Each message from a profile \n"
                                        "will be prefixed as follows:")
                                 .arg(TDebug::smIdentifierMap.count()));

    return messageLines.join(QChar::LineFeed).append(QChar::LineFeed);
}

/* static */ void TDebug::flushMessageQueue()
{
    TDebug localMessage(Qt::black, Qt::white);
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
            addHost(pHost);
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
