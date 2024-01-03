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

// Returns true if install is successful
// Tries to install a package on already-running mudlet instance.
bool MudletServer::tryInstall(const QString& packageName)
{
    // Pass the absolute path of the package to the active Mudlet Server 
    // The Mudlet Server may be owned by this process or another process.
    QLocalSocket socket;
    const QString absolutePackagePath = QDir(packageName).absolutePath();
    socket.connectToServer(mServerName);
    qDebug() << "connecting to server";

    if (socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        qDebug() << "connected, will send package name: " << absolutePackagePath;
        socket.write(absolutePackagePath.toUtf8());
        socket.waitForBytesWritten(WAIT_FOR_RESPONSE_MS);
        socket.disconnectFromServer();
        return true; 
    }
    qDebug() << "Could not connect to server after " << WAIT_FOR_RESPONSE_MS << "ms";
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
void MudletServer::tryInstallQueuedPackages(Host* activeProfile)
{
    qDebug() << "InstallQueue Started";
    foreach (const QString &path, mQueuedPackagePaths) {
        auto ret = activeProfile->installPackage(path,0);
        if (ret.first) {
            const QString infoMsg = tr("Package \"%1\" has been installed.").arg(path);
            activeProfile->postMessage(infoMsg);
        } else {
            const QString infoMsg = tr("Error: Package \"%1\" was not installed.").arg(path);
            activeProfile->postMessage(infoMsg);
        }
        //qApp->processEvents();
    }
    mQueuedPackagePaths.clear();
    qDebug() << "InstallQueue Finished";
}

void MudletServer::incomingConnection(quintptr socketDescriptor)
{

    qDebug() << "incomingConnection()";
    QLocalSocket* socket = new QLocalSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QLocalSocket::readyRead, this, &MudletServer::handleReadyRead);
    connect(socket, &QLocalSocket::disconnected, this, &MudletServer::handleDisconnected);
}

// Read a package path from socket and attempt to install it
void MudletServer::handleReadyRead()
{

    qDebug() << "handleReadyRead()";
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    // Add the package path to the install queue.
    qDebug() << "Adding package to install queue";
    QByteArray data = socket->readAll();
    QString packageName = QString::fromUtf8(data);
    mQueuedPackagePaths << packageName;

    // If mudlet is not running, stop here.
    qDebug() << "checking if mudlet is running";
    mudlet* mudletApp = mudlet::self();
    if(!mudletApp){
        return;
    }

    qDebug() << "checking if mudlet has active host";
    // Check if a profile (host) is active.
    Host* activeHost = mudletApp->getActiveHost();
    if(activeHost){
        // Try to install all packages in queue.
        qDebug() << "trying to install" << activeHost->getName();
        //activeHost->resetProfile_phase1();
        tryInstallQueuedPackages(activeHost);
        //activeHost->resetProfile_phase1();
        //mudletApp->activateProfile(activeHost);
        //mudletApp->slot_showConnectionDialog();
    } else {
        // Prompt user to select a profile.
        qDebug() << "show connection dialog";
        mudletApp->slot_showConnectionDialog();
    }
    qDebug() << "handleReadReady() Complete";
}

void MudletServer::handleDisconnected()
{
    qDebug() << "handleDisconnected()";
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
    qDebug() << "handleDisconnected() end";
}
