/***************************************************************************
 *   Copyright (C) 2023-2023 by Adam Robinson - seldon1951@hotmail.com     *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "MudletInstanceCoordinator.h"
#include "Host.h"
#include "mudlet.h"
#include <QLocalSocket>

const int WAIT_FOR_RESPONSE_MS = 500;

MudletInstanceCoordinator::MudletInstanceCoordinator(const QString& serverName, QObject* parent)
: QLocalServer(parent)
, mServerName(serverName)
{
}

void MudletInstanceCoordinator::queuePackage(const QString& packageName)
{
    mQueuedPackagePaths << packageName;
}

// Install the package queue on another instance of Mudlet.
// Returns true on success
bool MudletInstanceCoordinator::installPackagesRemotely()
{
    QLocalSocket socket;
    socket.connectToServer(mServerName);

    if (socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        const QString packagePathsData = mQueuedPackagePaths.join(QChar::LineFeed) + QChar::LineFeed;
        if (socket.write(packagePathsData.toUtf8()) == -1) {
            return false;
        }
        if (!socket.waitForBytesWritten(WAIT_FOR_RESPONSE_MS)) {
            return false;
        }
        socket.disconnectFromServer();
        return true;
    }
    return false;
}


// Attempt to start the Mudlet Server, return true if successful.
// If a server was already running, cancel the attempt and return false.
bool MudletInstanceCoordinator::tryToStart()
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
void MudletInstanceCoordinator::installPackagesToHost(Host* activeProfile)
{
    mMutex.lock();
    for (const QString& path : std::as_const(mQueuedPackagePaths)) {
        auto ret = activeProfile->installPackage(path, enums::PackageModuleType::Package);
    }
    mQueuedPackagePaths.clear();
    mMutex.unlock();
}

void MudletInstanceCoordinator::incomingConnection(quintptr socketDescriptor)
{
    QLocalSocket* socket = new QLocalSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QLocalSocket::readyRead, this, &MudletInstanceCoordinator::handleReadyRead);
    connect(socket, &QLocalSocket::disconnected, this, &MudletInstanceCoordinator::handleDisconnected);
}

void MudletInstanceCoordinator::handleReadyRead()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    mSocketBuffers[socket].append(socket->readAll());
    QByteArray& buffer = mSocketBuffers[socket];

    // Process complete newline-terminated messages
    for (int newlineIdx = buffer.indexOf('\n'); newlineIdx != -1; newlineIdx = buffer.indexOf('\n')) {
        const QString message = QString::fromUtf8(buffer.left(newlineIdx));
        buffer.remove(0, newlineIdx + 1);

        if (message.startsWith(qsl("TELNET_URI:"))) {
            const QString uri = message.mid(11);

            QTimer::singleShot(0, this, [uri]() {
                mudlet* app = mudlet::self();
                if (app) {
                    app->handleTelnetUri(uri);
                }
            });
        } else {
            QMutexLocker locker(&mMutex);
            if (!message.isEmpty()) {
                mQueuedPackagePaths << message;
            }
            locker.unlock();

            installPackagesLocally();
        }
    }
}

// Find the active host and install queued packages to it
void MudletInstanceCoordinator::installPackagesLocally()
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


QStringList MudletInstanceCoordinator::readPackageQueue()
{
    return mQueuedPackagePaths;
}

void MudletInstanceCoordinator::handleDisconnected()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        mSocketBuffers.remove(socket);
        socket->deleteLater();
    }
}

void MudletInstanceCoordinator::queueTelnetUri(const QString& uri)
{
    QMutexLocker locker(&mMutex);
    mQueuedTelnetUri = uri;
}

bool MudletInstanceCoordinator::forwardTelnetUriToRunningInstance()
{
    QString uri;
    {
        QMutexLocker locker(&mMutex);
        if (mQueuedTelnetUri.isEmpty()) {
            return false;
        }
        uri = mQueuedTelnetUri;
    }

    QLocalSocket socket;
    socket.connectToServer(mServerName);

    if (!socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        return false;
    }

    const QByteArray data = qsl("TELNET_URI:%1\n").arg(uri).toUtf8();
    if (socket.write(data) == -1) {
        return false;
    }
    if (!socket.waitForBytesWritten(WAIT_FOR_RESPONSE_MS)) {
        return false;
    }

    socket.disconnectFromServer();

    QMutexLocker locker(&mMutex);
    mQueuedTelnetUri.clear();
    return true;
}

