/***************************************************************************
 *   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@@gmail.com        *
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

#include <QPointer>
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QDebug>

#include "TelnetServerStub.h"
#include "utils.h"

#include <chrono>

using namespace std::chrono_literals;

TelnetServerStub::TelnetServerStub(QObject* parent)
: QTcpServer(parent)
{
    connect(this, &QTcpServer::newConnection, this, &TelnetServerStub::onNewConnection);
}

void TelnetServerStub::start(const QString& host, quint16 port)
{
    Q_UNUSED(host)
    const QHostAddress addr = QHostAddress::LocalHost;
    if (listen(addr, port)) {
        qInfo().noquote() << qsl("✅ TelnetServerStub listening on %1:%2").arg(addr.toString()).arg(serverPort());
    } else {
        qCritical().noquote() << qsl("❌ Failed to start TelnetServerStub: %1").arg(errorString());
    }
}

TelnetServerStub::~TelnetServerStub()
{
    if (!mPendingData.isEmpty()) {
        qWarning().noquote() << qsl("⚠️ TelnetServerStub destroyed with %1 undelivered queued bytes - no client was ever accepted.").arg(mPendingData.size());
    }
}

void TelnetServerStub::sendRaw(const QByteArray& data)
{
    if (!mpClient) {
        if (mHadClient) {
            // Between sessions there is no race to absorb, so queuing here
            // would leak a dead session's bytes into the next connection:
            qWarning() << "⚠️ sendRaw called without a connected client.";
            return;
        }
        // Tests wait on the client-side connected signal, which can fire
        // before this server side has accepted the connection - dropping the
        // bytes here loses the payload on fast runners, so hold them until
        // onNewConnection():
        mPendingData.append(data);
        return;
    }
    mpClient->write(data);
    mpClient->flush();
}

void TelnetServerStub::onNewConnection()
{
    QTcpSocket* client = nextPendingConnection();

    if (!client) {
        qWarning() << "⚠️ onNewConnection called but no pending connection.";
        return;
    }
    mpClient = client;
    mHadClient = true;
    mReceived.clear();
    qInfo().noquote() << qsl("🔌 Client connected: %1").arg(client->peerAddress().toString());

    if (!mPendingData.isEmpty()) {
        const auto bytesWritten = client->write(mPendingData);
        client->flush();
        if (bytesWritten <= 0) {
            qWarning().noquote() << qsl("⚠️ Failed to deliver %1 queued bytes to %2").arg(QString::number(mPendingData.size()), client->peerAddress().toString());
        }
        mPendingData.clear();
    }

    QPointer<QTcpSocket> safeClient = client;

    QTimer::singleShot(100ms, [safeClient, welcomeMessage = mpWelcomeMessage]() {
        if (!safeClient) {
            return;
        }
        const auto bytesWritten = safeClient->write(welcomeMessage.toUtf8() + "\r\n");
        safeClient->flush();
        if (bytesWritten <= 0) {
            qWarning().noquote() << qsl("⚠️ Failed to send welcome message to %1").arg(safeClient->peerAddress().toString());
        }
    });

    connect(client, &QTcpSocket::readyRead, this, [this, client]() {
        collectNawsUpdates(client);
    });

    // ~QTcpSocket emits disconnected() from ~QAbstractSocket, by which point the
    // socket is no longer a QTcpSocket - so hold it as the base type here.
    connect(client, &QTcpSocket::disconnected, [safeSocket = QPointer<QAbstractSocket>(client)]() {
        if (!safeSocket) {
            return;
        }
        qInfo().noquote() << qsl("Client disconnected: %1").arg(safeSocket->peerAddress().toString());
        safeSocket->deleteLater();
    });
}

void TelnetServerStub::collectNawsUpdates(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }
    mReceived.append(socket->readAll());

    // IAC SB NAWS <width hi> <width lo> <height hi> <height lo> IAC SE, with any
    // of those four bytes sent twice when it is 0xFF - which a width or height
    // whose low byte is 255 is enough to produce - so the frame is not a fixed
    // nine bytes and cannot be stepped over as though it were.
    static const QByteArray nawsHeader = QByteArray("\xFF\xFA\x1F", 3);
    static const QByteArray nawsFooter = QByteArray("\xFF\xF0", 2);
    qsizetype at = 0;
    qsizetype keepFrom = -1;
    while ((at = mReceived.indexOf(nawsHeader, at)) >= 0) {
        qsizetype cursor = at + nawsHeader.size();
        int payload[4] = {};
        bool whole = true;
        for (int& byte : payload) {
            if (cursor >= mReceived.size()) {
                whole = false;
                break;
            }
            byte = static_cast<unsigned char>(mReceived.at(cursor++));
            if (byte == 0xFF) {
                // the doubled half, which carries no value of its own
                ++cursor;
            }
        }
        if (!whole || cursor + nawsFooter.size() > mReceived.size()) {
            // the rest of it is still in flight - hold this frame's bytes back
            keepFrom = at;
            break;
        }
        if (mReceived.mid(cursor, nawsFooter.size()) != nawsFooter) {
            // Not a subnegotiation this stub understands. Skipping the header
            // rather than a whole frame keeps the scan aligned on whatever
            // really is a NAWS update further along.
            qWarning() << "TelnetServerStub: a NAWS subnegotiation did not end in IAC SE - ignoring it";
            at += nawsHeader.size();
            continue;
        }
        mNawsUpdates.append(QSize(payload[0] * 256 + payload[1], payload[2] * 256 + payload[3]));
        at = cursor + nawsFooter.size();
    }
    // Everything the client sends comes through here, so hold back only what a
    // frame still being delivered needs: the part of it that has arrived, or
    // failing that the couple of bytes that could be a header cut in half.
    mReceived = keepFrom >= 0 ? mReceived.mid(keepFrom) : mReceived.right(nawsHeader.size() - 1);
}
