/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include "DiscordIpcServerStub.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QtEndian>

#include "utils.h"

// The discord-rpc client only ever tries discord-ipc-0 through discord-ipc-9;
// a fresh runtime directory guarantees slot 0 is free:
static const QString scSocketFileName = qsl("discord-ipc-0");

DiscordIpcServerStub::DiscordIpcServerStub(QObject* parent)
: QObject(parent)
{
    connect(&mServer, &QLocalServer::newConnection, this, &DiscordIpcServerStub::onNewConnection);
}

DiscordIpcServerStub::~DiscordIpcServerStub()
{
    // The sockets are children of mServer, which is destroyed after mBuffers;
    // stop their destruction-time disconnected signals from reaching the
    // lambdas that touch mBuffers:
    const auto sockets = mBuffers.keys();
    for (QLocalSocket* socket : sockets) {
        socket->disconnect(this);
    }
    mServer.close();
}

bool DiscordIpcServerStub::start(const QString& username)
{
    mUsername = username;

#if defined(Q_OS_WIN)
    // On Windows QLocalServer creates the named pipe \\.\pipe\discord-ipc-0,
    // which is exactly where the discord-rpc library looks - no runtime
    // directory needed (or possible):
    const QString socketPath = scSocketFileName;
#else
    mpRuntimeDir = std::make_unique<QTemporaryDir>();
    if (!mpRuntimeDir->isValid()) {
        qCritical().noquote() << qsl("DiscordIpcServerStub: could not create a runtime directory: %1").arg(mpRuntimeDir->errorString());
        return false;
    }
    const QString socketPath = qsl("%1/%2").arg(mpRuntimeDir->path(), scSocketFileName);
    // sizeof(sockaddr_un::sun_path) is 108 on Linux (104 on macOS):
    if (socketPath.size() >= 100) {
        qCritical().noquote() << qsl("DiscordIpcServerStub: socket path \"%1\" is too long for AF_UNIX").arg(socketPath);
        return false;
    }
#endif

    if (!mServer.listen(socketPath)) {
        qCritical().noquote() << qsl("DiscordIpcServerStub: could not listen on \"%1\": %2").arg(socketPath, mServer.errorString());
        return false;
    }
    qInfo().noquote() << qsl("DiscordIpcServerStub: listening on \"%1\" as Discord user \"%2\"").arg(mServer.fullServerName(), mUsername);
    return true;
}

QString DiscordIpcServerStub::runtimeDir() const
{
    return mpRuntimeDir ? mpRuntimeDir->path() : QString();
}

void DiscordIpcServerStub::onNewConnection()
{
    while (QLocalSocket* socket = mServer.nextPendingConnection()) {
        connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
            mBuffers[socket].append(socket->readAll());
            processBuffer(socket);
        });
        connect(socket, &QLocalSocket::disconnected, this, [this, socket]() {
            mBuffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void DiscordIpcServerStub::processBuffer(QLocalSocket* socket)
{
    QByteArray& buffer = mBuffers[socket];
    while (buffer.size() >= 8) {
        const quint32 opcode = qFromLittleEndian<quint32>(buffer.constData());
        const quint32 length = qFromLittleEndian<quint32>(buffer.constData() + 4);
        if (static_cast<quint64>(buffer.size()) < 8 + static_cast<quint64>(length)) {
            return;
        }
        const QByteArray payload = buffer.mid(8, static_cast<qsizetype>(length));
        buffer.remove(0, 8 + static_cast<qsizetype>(length));

        switch (opcode) {
        case 0: // HANDSHAKE - reply with the READY dispatch
            ++mHandshakeCount;
            mHandshakePayloads << QString::fromUtf8(payload);
            sendFrame(socket, 1, readyPayload());
            break;
        case 1: // FRAME - e.g. SET_ACTIVITY; record it for assertions
            mFramePayloads << QString::fromUtf8(payload);
            break;
        case 3: // PING - reply with PONG
            sendFrame(socket, 4, payload);
            break;
        default: // CLOSE (2) or unexpected - nothing to do
            break;
        }
    }
}

void DiscordIpcServerStub::sendFrame(QLocalSocket* socket, quint32 opcode, const QByteArray& payload)
{
    QByteArray frame;
    frame.reserve(8 + payload.size());
    const quint32 leOpcode = qToLittleEndian(opcode);
    const quint32 leLength = qToLittleEndian(static_cast<quint32>(payload.size()));
    frame.append(reinterpret_cast<const char*>(&leOpcode), sizeof(leOpcode));
    frame.append(reinterpret_cast<const char*>(&leLength), sizeof(leLength));
    frame.append(payload);
    socket->write(frame);
    socket->flush();
}

QByteArray DiscordIpcServerStub::readyPayload() const
{
    const QJsonObject user{{qsl("id"), qsl("111111111111111111")},
                           {qsl("username"), mUsername},
                           {qsl("discriminator"), qsl("0")},
                           {qsl("global_name"), mUsername},
                           {qsl("avatar"), QJsonValue()},
                           {qsl("bot"), false},
                           {qsl("flags"), 0},
                           {qsl("premium_type"), 0}};
    const QJsonObject config{{qsl("cdn_host"), qsl("cdn.discordapp.com")}, {qsl("api_endpoint"), qsl("//discord.com/api")}, {qsl("environment"), qsl("production")}};
    const QJsonObject data{{qsl("v"), 1}, {qsl("config"), config}, {qsl("user"), user}};
    const QJsonObject ready{{qsl("cmd"), qsl("DISPATCH")}, {qsl("evt"), qsl("READY")}, {qsl("data"), data}};
    return QJsonDocument(ready).toJson(QJsonDocument::Compact);
}
