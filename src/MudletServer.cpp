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

#include "Host.h"
#include "mudlet.h"
#include "MudletServer.h"
#include <QLocalSocket>
#include <QDebug>

const int WAIT_FOR_RESPONSE_MS = 500;


// Keeps note of the package that the user want to install
// This method is called when Mudlet is ran with the --package flag
// or when a .mpackage file is opened
void MudletServer::queuePackage(const QString& packageName)
{
    mQueuedPackagePaths << packageName;
}

// Attempts to install the package remotely 
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


MudletServer::MudletServer(const QString& serverName, QObject* parent) : QLocalServer(parent), mServerName(serverName)
{
}

// Attempt to start the Mudlet Server, but not if one is already running.
bool MudletServer::tryToStart()
{
    QLocalSocket socket;
    socket.connectToServer(mServerName);

    // If there is no response within a reasonable time:
    // This process will be responsible for managing the server. 
    if (!socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        qDebug() << "Server started";
        listen(mServerName);
        return true;
    }
    return false;
}

// Install all queued packages to a specified profile.
// Mudlet calls this function whenever a profile is activated.
// Returns true if no installations fail
void MudletServer::installPackagesLocally(Host* activeProfile)
{

    mMutex.lock();
    foreach (const QString &path, mQueuedPackagePaths) {
        auto ret = activeProfile->installPackage(path,0);
        if (ret.first) {
            const QString infoMsg = tr("Package \"%1\" has been installed.").arg(path);
            activeProfile->postMessage(infoMsg);
        } else {
            const QString infoMsg = tr("Error: Package \"%1\" was not installed.").arg(path);
            activeProfile->postMessage(infoMsg);
        }
    }
    mQueuedPackagePaths.clear();
    mMutex.unlock();
    return true;
}

void MudletServer::incomingConnection(quintptr socketDescriptor)
{
    QLocalSocket* socket = new QLocalSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QLocalSocket::readyRead, this, &MudletServer::handleReadyRead);
    connect(socket, &QLocalSocket::disconnected, this, &MudletServer::handleDisconnected);
}

// Receive package paths and attempt to install them 
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

    // Try to install the packages
    QTimer::singleShot(0, this, [this]() {
        mudlet* mudletApp = mudlet::self();
        Q_ASSERT(mudletApp);
        Host* activeHost = mudletApp->getActiveHost();
        if(activeHost){
            installPackagesLocally(activeHost);
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
