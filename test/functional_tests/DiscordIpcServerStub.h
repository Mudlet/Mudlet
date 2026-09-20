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

#ifndef DISCORD_IPC_SERVER_STUB_H
#define DISCORD_IPC_SERVER_STUB_H

#include <QHash>
#include <QLocalServer>
#include <QStringList>

#include <memory>

class QLocalSocket;
class QTemporaryDir;

// Emulates the local IPC endpoint of a running Discord client, just enough
// for the bundled discord-rpc library to complete its handshake and believe
// a user is logged in.
//
// Wire framing: [opcode:uint32 LE][length:uint32 LE][json payload]
//   opcode 0 = HANDSHAKE, 1 = FRAME, 2 = CLOSE, 3 = PING, 4 = PONG
//
// On HANDSHAKE it replies with a FRAME carrying a DISPATCH/READY event whose
// data.user.username is the configured fake account - that is what ends up
// in Discord::smUserName via the library's ready callback. Every FRAME
// received afterwards (e.g. SET_ACTIVITY presence updates) is recorded for
// tests to assert on.
class DiscordIpcServerStub : public QObject
{
    Q_OBJECT

public:
    explicit DiscordIpcServerStub(QObject* parent = nullptr);
    ~DiscordIpcServerStub() override;

    // Creates a fresh, short runtime directory and listens as discord-ipc-0
    // inside it. AF_UNIX socket paths are limited to ~108 characters, hence
    // the deliberately short directory. Point XDG_RUNTIME_DIR at
    // runtimeDir() before the discord-rpc library makes its first
    // connection attempt.
    bool start(const QString& username);

    bool listening() const { return mServer.isListening(); }
    QString runtimeDir() const;

    int handshakeCount() const { return mHandshakeCount; }
    QStringList handshakePayloads() const { return mHandshakePayloads; }
    QStringList framePayloads() const { return mFramePayloads; }
    void clearRecordedFrames() { mFramePayloads.clear(); }

private slots:
    void onNewConnection();

private:
    void processBuffer(QLocalSocket* socket);
    void sendFrame(QLocalSocket* socket, quint32 opcode, const QByteArray& payload);
    QByteArray readyPayload() const;

    QLocalServer mServer;
    QString mUsername;
    std::unique_ptr<QTemporaryDir> mpRuntimeDir;
    QHash<QLocalSocket*, QByteArray> mBuffers;
    int mHandshakeCount = 0;
    QStringList mHandshakePayloads;
    QStringList mFramePayloads;
};

#endif // DISCORD_IPC_SERVER_STUB_H
