#ifndef MUDLET_RECORDINGTELNETSERVER_H
#define MUDLET_RECORDINGTELNETSERVER_H

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
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "ctelnet.h"

// TelnetServerStub speaks; this one only listens, so a test can assert on the
// bytes a game would have received. Everything a client sends is kept, since
// several out-of-band protocols (NEW_ENVIRON above all) are only observable in
// what Mudlet replies with.
//
// Deliberately no Q_OBJECT: there is no matching .cpp for AUTOMOC to attach a moc
// step to (TelnetServerStub.h gets away with it only because TelnetServerStub.cpp
// is in FUNCTIONAL_TEST_UTILS), and the class declares no signals or slots of its
// own, so connecting to lambdas with itself as context is enough. Adding a signal
// later would fail at link rather than at compile.
class RecordingTelnetServer : public QObject
{
public:
    explicit RecordingTelnetServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket* socket = mServer.nextPendingConnection();
            if (!socket) {
                return;
            }
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                mReceived.append(socket->readAll());
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
    }

    // Port 0: an ephemeral port, so concurrent test runs cannot collide. An
    // unchecked failure here reads much later as a connection that never
    // arrives, since serverPort() then answers 0.
    [[nodiscard]] bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    QByteArray received() const { return mReceived; }
    void forgetReceived() { mReceived.clear(); }

private:
    // The buffer is declared first so it outlives the server, and with it the
    // accepted sockets whose read handlers write into it.
    QByteArray mReceived;
    QTcpServer mServer;
};

// One IAC SB ... IAC SE block, with the escaped IAC pairs collapsed.
struct Subnegotiation
{
    unsigned char option = 0;
    QByteArray payload;
};

// Every subnegotiation in a captured stream, in the order it was sent. Ordinary
// data around them is skipped, including the doubled IAC Mudlet writes for a
// literal 0xFF - which would otherwise read as the start of a block whenever the
// byte after it happened to be SB.
inline QList<Subnegotiation> subnegotiationsIn(const QByteArray& stream)
{
    const auto byteAt = [&stream](const int index) {
        return static_cast<unsigned char>(stream.at(index));
    };
    constexpr auto iac = static_cast<unsigned char>(TN_IAC);
    constexpr auto sb = static_cast<unsigned char>(TN_SB);
    constexpr auto se = static_cast<unsigned char>(TN_SE);

    QList<Subnegotiation> found;
    int i = 0;
    while (i + 1 < stream.size()) {
        if (byteAt(i) != iac) {
            ++i;
            continue;
        }
        if (byteAt(i + 1) == iac) {
            i += 2;
            continue;
        }
        if (byteAt(i + 1) != sb) {
            ++i;
            continue;
        }
        if (i + 2 >= stream.size()) {
            break;
        }
        Subnegotiation subnegotiation;
        subnegotiation.option = byteAt(i + 2);
        int j = i + 3;
        bool terminated = false;
        bool truncated = false;
        while (j < stream.size()) {
            if (byteAt(j) == iac) {
                if (j + 1 >= stream.size()) {
                    break;
                }
                const unsigned char next = byteAt(j + 1);
                if (next == se) {
                    terminated = true;
                    j += 2;
                    break;
                }
                if (next == iac) {
                    subnegotiation.payload.append(static_cast<char>(TN_IAC));
                    j += 2;
                    continue;
                }
                if (next == sb) {
                    // The next block has begun, so this one lost its terminator:
                    // drop it rather than swallowing its successor's bytes.
                    truncated = true;
                    break;
                }
            }
            subnegotiation.payload.append(stream.at(j));
            ++j;
        }
        if (truncated) {
            i = j;
            continue;
        }
        if (!terminated) {
            // A subnegotiation still arriving: leave it for the next capture
            // rather than reporting a payload that has not finished.
            break;
        }
        found.append(subnegotiation);
        i = j;
    }
    return found;
}

inline QList<Subnegotiation> subnegotiationsFor(const QByteArray& stream, const unsigned char option)
{
    QList<Subnegotiation> found;
    for (const Subnegotiation& subnegotiation : subnegotiationsIn(stream)) {
        if (subnegotiation.option == option) {
            found.append(subnegotiation);
        }
    }
    return found;
}

#endif // MUDLET_RECORDINGTELNETSERVER_H
