/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2021 by Stephen Lyons - slysven@virginmedia.com   *
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
#include "mudlet.h"

// This is a Unicode NON-character code which is explicitly undisplayable but
// can be embedded for our own internal purposes - in this case to flag that
// the text is a continuation of a previous one and should NOT have a marking
// prepended to indicate from which profile it came.
const QChar TDebug::csmContinue = QChar(0xFFFF);

QMap<const Host*, QPair<QString, QChar>> TDebug::smIdentifierMap;
QSet<QChar> TDebug::smUsedIdentifiers;
QQueue<TDebugMessage> TDebug::smMessageQueue;

TDebug::TDebug(const QColor& c, const QColor& d)
: fgColor(c)
, bgColor(d)
{
}

// This is the method that pushes the accumulated text out to the Central Debug
// Console. If the accumulated message text (in msg) begins with csmContinue
// then this message is appended onto a previous one and should NOT include
// any indicator (at the beginning) of the profile from which it came - which
// is now deduced from the supplied Host pointer.
TDebug& TDebug::operator>>(Host* pHost)
{
    if (Q_UNLIKELY(!mudlet::mpDebugConsole)) {
        if (Q_LIKELY(!msg.isEmpty())) {
            // Don't enqueue empty messages
            QChar tag = deduceProfileTag(msg, pHost);
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
                    mudlet::mpDebugConsole->print(QStringLiteral("[%1] %2").arg(message.mTag, message.mMessage), message.mForeground, message.mBackground);
                }
            }
        }

        QChar tag = deduceProfileTag(msg, pHost);
        if (tag.isNull()) {
            // We use an empty message with no host pointer to flush out the
            // enqueued messages the first time the CDC is shown - so in that
            // case we will already done everything needed in previous chunk
            // of code. Otherwise just print the message without a tag marking:
            if (!msg.isEmpty()) {
                mudlet::mpDebugConsole->print(msg, fgColor, bgColor);
            }
        } else if (tag == QLatin1Char('*') || Q_UNLIKELY(tag == QLatin1Char('!')) || TDebug::smIdentifierMap.count() > 1) {
            // More than one profile active or this is a system message or something went wrong in identifying the profile:
            mudlet::mpDebugConsole->print(QStringLiteral("[%1] %2").arg(tag, msg), fgColor, bgColor);
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
        QPair<QString, QChar>& pair = TDebug::smIdentifierMap[pHost];
        pair.first = newName;
    }
}

/* static */ void TDebug::addHost(Host* pHost)
{
    if (!pHost) {
        return;
    }
    static const QString validIdentifiers{"123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
    static const int maxIdCount = validIdentifiers.size();
    int tryCount = -1;
    do {
        if (!TDebug::smUsedIdentifiers.contains(validIdentifiers.at(++tryCount))) {
            break;
        }
    } while (tryCount < maxIdCount);

    QString hostName = pHost->getName();
    if (TDebug::smUsedIdentifiers.contains(validIdentifiers.at(tryCount))) {
        // Run out of identifers - fall back to '?'
        QPair<QString, QChar> newIdentifier = qMakePair(hostName, QLatin1Char('?'));
        TDebug::smIdentifierMap.insert(pHost, newIdentifier);
    } else {
        QPair<QString, QChar> newIdentifier = qMakePair(hostName, validIdentifiers.at(tryCount));
        TDebug::smUsedIdentifiers.insert(validIdentifiers.at(tryCount));
        TDebug::smIdentifierMap.insert(pHost, newIdentifier);
    }
    TDebug localMessage(Qt::blue, Qt::white);
    localMessage << QStringLiteral("Profile: '%1' started.\n").arg(hostName) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black);
    tableMessage << TDebug::displayNewTable() >> nullptr;
}

/* static */ void TDebug::removeHost(Host* pHost)
{
    TDebug::smIdentifierMap.remove(pHost);
    TDebug localMessage(Qt::darkGray, Qt::white);
    localMessage << QStringLiteral("Profile: '%1' ended.\n").arg(pHost->getName()) >> nullptr;
    TDebug tableMessage(Qt::white, Qt::black);
    tableMessage << TDebug::displayNewTable() >> nullptr;
}

/* static */ QString TDebug::displayNewTable()
{
    // We do not translate this currently as the Central Debug Console does not
    // get translated content - yet?
    QStringList messageLines;
    QMapIterator<const Host*, QPair<QString, QChar>> itIdentifier(TDebug::smIdentifierMap);
    while (itIdentifier.hasNext()) {
        itIdentifier.next();
        if (itIdentifier.key()) {
            messageLines.append(QStringLiteral(" [%1] = \"%2\"").arg(itIdentifier.value().second, itIdentifier.value().first));
        }
    }
    if (messageLines.count() > 1) {
        std::sort(messageLines.begin(), messageLines.end());
    }
    messageLines.prepend(QStringLiteral(" [*] = System message, not belonging to a specific profile"));
    if (TDebug::smIdentifierMap.count() > 1) {
        messageLines.prepend(QStringLiteral("There are %1 profiles active now, so each message from a profile will be "
                                            "prefixed with a single character inside a pair of '['...']'s as follows:").arg(TDebug::smIdentifierMap.count()));
    } else {
        messageLines.prepend(QStringLiteral("There is only a single profile active right now, so only messages not from that "
                                            "profile will be prefixed though the indicated character will be retained and "
                                            "used should another profile be opened:"));
    }

    return messageLines.join(QChar::LineFeed).append(QChar::LineFeed);
}

/* static */ void TDebug::flushMessageQueue()
{
    TDebug localMessage(Qt::black, Qt::white);
    localMessage << QString() >> nullptr;
}

// This will strip the TDebug::csmContinue QChar from the start of text:
/* static */ QChar TDebug::deduceProfileTag(QString& text, Host* pHost)
{
    if (text.startsWith(csmContinue)) {
        text.remove(0, 1);
        return QChar();
    }
    if (pHost) {
        if (!smIdentifierMap.contains(pHost)) {
            // Oops, we do not have that Host on file, better create something
            // for it - this will also cause a pair of new TDebug messages to
            // be created and processed prior to the call to this method being
            // completed:
            addHost(pHost);
        }
        // By now smIdentifierMap WILL contain something for pHost - but use '!'
        // if something is really screwy and it does not:
        return smIdentifierMap.value(pHost, qMakePair(QString(), QLatin1Char('!'))).second;
    }
    // Must be a system message - or the dummy one to flush the queue.
    if (!text.isEmpty()) {
        // A system message:
        return QLatin1Char('*');
    }
    // The dummy one:
    return QChar();
}
