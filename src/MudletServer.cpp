/***************************************************************************
 *   Copyright (C) 2023-2023 by Adam Robinson - seldon1951@hotmail.com     *
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

#include "MudletServer.h"
#include "Host.h"
#include "mudlet.h"
#include <QDebug>
#include <QLocalSocket>

const int WAIT_FOR_RESPONSE_MS = 500;

MudletServer::MudletServer(const QString& serverName, QObject* parent) : QLocalServer(parent), mServerName(serverName) {}

void MudletServer::queuePackage(const QString& packageName)
{
    mQueuedPackagePaths << packageName;
}

// Install the package queue on another instance of Mudlet.
// Returns true on success
bool MudletServer::installPackagesRemotely()
{
    // Pass the absolute path of the package to the active Mudlet Server
    // The Mudlet Server may be owned by this process or another process.
    QLocalSocket socket;
    socket.connectToServer(mServerName);

    if (socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        const QString packagePathsData = mQueuedPackagePaths.join(QChar::LineFeed);
        socket.write(packagePathsData.toUtf8());
        socket.waitForBytesWritten(WAIT_FOR_RESPONSE_MS);
        socket.disconnectFromServer();
        return true;
    }
    return false;
}


// Attempt to start the Mudlet Server, return true if successful.
// If a server was already running, cancel the attempt and return false.
bool MudletServer::tryToStart()
{
    QLocalSocket socket;
    socket.connectToServer(mServerName);

    // If there is no response within a reasonable time:
    // Assume no server exists, and start a new server.
    if (!socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        listen(mServerName);
        return true;
    }
    return false;
}

// Install all queued packages to a specified profile/host.
// Mudlet will call this function whenever a profile is activated.
void MudletServer::installPackagesToHost(Host* activeProfile)
{
    mMutex.lock();
    foreach(const QString& path, mQueuedPackagePaths) {
        auto ret = activeProfile->installPackage(path, 0);
    }
    mQueuedPackagePaths.clear();
    mMutex.unlock();
}

void MudletServer::incomingConnection(quintptr socketDescriptor)
{
    QLocalSocket* socket = new QLocalSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QLocalSocket::readyRead, this, &MudletServer::handleReadyRead);
    connect(socket, &QLocalSocket::disconnected, this, &MudletServer::handleDisconnected);
}

// Receive package paths and install them
void MudletServer::handleReadyRead()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    mMutex.lock();
    // Receive package paths and add them the install queue.
    QByteArray data = socket->readAll();
    const QStringList packagePaths = QString::fromUtf8(data).split(QChar::LineFeed, Qt::SkipEmptyParts);
    mQueuedPackagePaths << packagePaths;
    mMutex.unlock();

    installPackagesLocally();
}

// Find the active host and install queued packages to it
void MudletServer::installPackagesLocally()
{
    QTimer::singleShot(0, this, [this]() {
        mudlet* mudletApp = mudlet::self();
        Q_ASSERT(mudletApp);
        Host* activeHost = mudletApp->getActiveHost();
        if (activeHost) {
            installPackagesToHost(activeHost);
        } else {
            mudletApp->slot_showConnectionDialog();
        }
    });
}

void MudletServer::handleDisconnected()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
}
