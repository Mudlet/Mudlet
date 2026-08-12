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

#ifndef TELNET_SERVER_STUB_H
#define TELNET_SERVER_STUB_H

#include <QPointer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QTimer>
#include <QDebug>

class TelnetServerStub : public QTcpServer
{
    Q_OBJECT

    QString mpWelcomeMessage = "";
    QPointer<QTcpSocket> mpClient;
    QByteArray mReceivedData;

public:
    explicit TelnetServerStub(QObject* parent = nullptr);

    // Bind to a caller-chosen port, or to an ephemeral OS-assigned port when port is 0 (the default).
    // Binding 0 lets concurrent test runs avoid colliding on a shared fixed port; the caller reads the
    // actual bound port back via serverPort() (inherited from QTcpServer) once start() has succeeded.
    void start(const QString& host, quint16 port = 0);
    void setWelcomeMessage(const QString& message) { mpWelcomeMessage = message; }
    // Sends bytes verbatim to the connected client - allows telnet control
    // bytes like IAC GA to be included:
    void sendRaw(const QByteArray& data);
    bool clientConnected() const { return !mpClient.isNull(); }
    // Everything the client has sent so far, so a test can assert on what
    // Mudlet put on the wire (telnet negotiation replies, for instance):
    QByteArray receivedData() const { return mReceivedData; }
    void clearReceivedData() { mReceivedData.clear(); }

private slots:
    void onNewConnection();
};

#endif // TELNET_SERVER_STUB_H
