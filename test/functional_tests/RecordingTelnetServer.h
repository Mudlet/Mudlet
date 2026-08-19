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
// Deliberately no Q_OBJECT: it declares no signals or slots of its own, so
// connecting to lambdas is enough and the header stays usable from more than one
// test in the same binary without a moc step.
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

    // Port 0: an ephemeral port, so concurrent test runs cannot collide.
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    QByteArray received() const { return mReceived; }
    void forgetReceived() { mReceived.clear(); }

private:
    QTcpServer mServer;
    QByteArray mReceived;
};

// One IAC SB ... IAC SE block, with the escaped IAC pairs collapsed.
struct Subnegotiation
{
    unsigned char option = 0;
    QByteArray payload;
};

// Every subnegotiation in a captured stream, in the order it was sent.
inline QList<Subnegotiation> subnegotiationsIn(const QByteArray& stream)
{
    QList<Subnegotiation> found;
    int i = 0;
    while (i + 1 < stream.size()) {
        if (static_cast<unsigned char>(stream.at(i)) != static_cast<unsigned char>(TN_IAC) || static_cast<unsigned char>(stream.at(i + 1)) != static_cast<unsigned char>(TN_SB)) {
            ++i;
            continue;
        }
        if (i + 2 >= stream.size()) {
            break;
        }
        Subnegotiation subnegotiation;
        subnegotiation.option = static_cast<unsigned char>(stream.at(i + 2));
        int j = i + 3;
        bool terminated = false;
        while (j < stream.size()) {
            if (static_cast<unsigned char>(stream.at(j)) == static_cast<unsigned char>(TN_IAC) && j + 1 < stream.size()) {
                const unsigned char next = static_cast<unsigned char>(stream.at(j + 1));
                if (next == static_cast<unsigned char>(TN_SE)) {
                    terminated = true;
                    j += 2;
                    break;
                }
                if (next == static_cast<unsigned char>(TN_IAC)) {
                    subnegotiation.payload.append(static_cast<char>(TN_IAC));
                    j += 2;
                    continue;
                }
            }
            subnegotiation.payload.append(stream.at(j));
            ++j;
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

// The subnegotiations for one option only.
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
