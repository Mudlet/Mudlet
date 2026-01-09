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
        qInfo().noquote() << qsl("✅ TelnetServerStub listening on %1:%2")
                    .arg(addr.toString())
                    .arg(port);
    } else {
        qCritical().noquote() << qsl("❌ Failed to start TelnetServerStub: %1").arg(errorString());
    }
}

void TelnetServerStub::onNewConnection()
{
    QTcpSocket* client = nextPendingConnection();

    if (!client) {
        qWarning() << "⚠️ onNewConnection called but no pending connection.";
        return;
    }
    qInfo().noquote() << qsl("🔌 Client connected: %1").arg(client->peerAddress().toString());

    QPointer<QTcpSocket> safeClient = client;

    QTimer::singleShot(100, [safeClient, welcomeMessage = mpWelcomeMessage]()
    {
        if (!safeClient) {
            return;
        }
        const auto bytesWritten = safeClient->write(welcomeMessage.toUtf8() + "\r\n");
        safeClient->flush();
        if (bytesWritten <= 0) {
            qWarning().noquote() << qsl("⚠️ Failed to send welcome message to %1")
                                    .arg(safeClient->peerAddress().toString());
        }
    });

    connect(client, &QTcpSocket::disconnected, [safeClient]()
    {
        if (!safeClient) {
            return;
        }
        qInfo().noquote() << qsl("Client disconnected: %1").arg(safeClient->peerAddress().toString());
        safeClient->deleteLater();
    });
}
