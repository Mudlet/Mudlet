#ifndef MUDLET_TDEBUG_H
#define MUDLET_TDEBUG_H

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


#include "pre_guard.h"
#include <QColor>
#include <QList>
#include <QMap>
#include <QQueue>
#include <QString>
#include "post_guard.h"

#include "utils.h"

class Host;

struct TDebugMessage
{
    TDebugMessage(const QString& text, const QString& tag, const QColor& fg, const QColor& bg)
    : mMessage(text)
    , mTag(tag)
    , mForeground(fg)
    , mBackground(bg)
    {}

    QString mMessage;
    QString mTag;
    QColor mForeground;
    QColor mBackground;

};

class TDebug
{
    // A shared map that is uses to put a short identifier on each debug message
    // - the first value is used to create a table to display on changes and the
    // second value is the short identifier is used:
    static QMap<const Host*, QPair<QString, QString>> smIdentifierMap;
    // Used to order identifier in the same application run:
    static QQueue<QString> smAvailableIdentifiers;
    static bool initialised;
    // This is a temporary bodge until we can decouple the Central Debug
    // Console from having to be associated with a Host (Profile) instance,
    // as that prevents it from being created until a profile has - which makes
    // displaying details from that first profile being loaded harder:
    static QQueue<TDebugMessage> smMessageQueue;

    // Used as a tag for system (non-profile) messages:
    // Change to use U+2731 {HEAVY ASTERIX} instead of an asterix:
    inline static const QString csmTagSystemMessage = qsl("[\u2731] ");
    // If something has gone wrong and it is not possible to work out which profile it is from,
    // don't use a tag. No need to expose the player to Mudlet internal details: fail gracefully
    inline static const QString csmTagFault = qsl("");
    // Used as a tag for messages on the 27th and above currently active profiles:
    inline static const QString csmTagOverflow  = qsl("[?] ");

    QString msg;
    QColor fgColor;
    QColor bgColor;

public:
    explicit TDebug(const QColor&, const QColor&);
    ~TDebug() = default;

    static void addHost(Host*);
    static void removeHost(Host*);
    static void changeHostName(const Host*, const QString&);
    static void flushMessageQueue();
    static QString getTag(Host*);

    // Used to flush/print out the accumulated message:
    TDebug& operator>>(Host*);

    // Used to append the argument type to the message:
    TDebug& operator<<(const QString&);
    TDebug& operator<<(const QChar&);
    TDebug& operator<<(const int&);
    // These should all be used with a preceding operator<<(const QString&)
    // that provides an opening '(' to match the one that these all append
    // after the content they add to the message:
    TDebug& operator<<(const QMap<QString, QString>&);
    TDebug& operator<<(const QMap<QString, int>&);
    TDebug& operator<<(const QMap<int, QString>&);
    TDebug& operator<<(const QMap<int, int>&);
    TDebug& operator<<(const QList<QString>&);
    TDebug& operator<<(const QList<int>&);

    // Prepend this to any continuation message to suppress the insertion of the
    // profile identifying marking.
    static const QChar csmContinue;


private:
    TDebug() = default;

    static QString displayNewTable();
    static QString deduceProfileTag(QString&, Host*);
};

#endif // MUDLET_TDEBUG_H
