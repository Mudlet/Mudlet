#ifndef MUDLET_TGAMETRANSPORT_H
#define MUDLET_TGAMETRANSPORT_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QByteArray>
#include <QObject>
#include <QString>

// A game connection made by the application embedding Mudlet in place of cTelnet's sockets, for a
// platform with no raw TCP such as a browser. It carries the telnet byte stream and nothing else:
// how the bytes reach the game is the application's business, so cTelnet never calls it secure.
// write() and close() are for the connection last dialled. Each signal carries the id dial() gave
// its connection, which no other dial in the process is given, and may be emitted at any time, even
// from inside dial(), write() or close(): cTelnet takes them up from the event loop and drops any
// not about the connection it last dialled and has not closed since, however late it comes.
class TGameTransport : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    // Answered by connected(), or by closed() if the game cannot be reached; tlsRequested is the
    // profile's wish for a secure connection, which only the application can act on
    virtual void dial(quint64 connection, const QString& host, int port, bool tlsRequested) = 0;
    virtual void write(const QByteArray& data) = 0;
    // Ends the connection or the attempt; cTelnet reports the end itself, so no closed() is due
    virtual void close() = 0;

signals:
    void connected(quint64 connection);
    void received(quint64 connection, const QByteArray& data);
    // The game closed the connection or it was lost, or a dial failed; reason may be empty
    void closed(quint64 connection, const QString& reason);
};

#endif // MUDLET_TGAMETRANSPORT_H
