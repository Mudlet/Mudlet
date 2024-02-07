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

#include "MudletInstanceCoordinator.h"
#include "Host.h"
#include "mudlet.h"
#include <QLocalSocket>
#include <qnamespace.h>

const int WAIT_FOR_RESPONSE_MS = 500;

MudletInstanceCoordinator::MudletInstanceCoordinator(const QString& serverName, QObject* parent) : QLocalServer(parent), mServerName(serverName) {}

void MudletInstanceCoordinator::queueUriOrFile(const QString& uriOrFile)
{
    QUrl uri(uriOrFile);
    if (uri.isValid() && !uri.scheme().isEmpty()) {
        // It's a URI if it has a valid scheme.
        queueUri(uri.toString());
        return;
    }

    QString file = uriOrFile;
    if (file.startsWith("~/")) {
        // Need to expand home directory
        file = QDir::homePath() + file.mid(1);
    }

    if (QFile::exists(file)) {
        // Comfirm file exists before adding to queue
        queueUri(QUrl::fromLocalFile(file));
        return;
    }

    qWarning() << "Tried to queue invalid URI or file:" << uriOrFile;
}
void MudletInstanceCoordinator::queueUri(const QUrl& uri)
{
    mQueuedUris << uri.toString();
}

QStringList MudletInstanceCoordinator::listUrisWithSchemes(const QStringList schemes)
{
    QStringList matchingUris;

    for (const QString& uri : mQueuedUris) {
        if (schemes.contains(QUrl(uri).scheme(), Qt::CaseInsensitive)) {
            matchingUris << uri;
        }
    }
    return matchingUris;
}

// Open queued URIs on another instance of Mudlet.
// Returns true on success
bool MudletInstanceCoordinator::openUrisRemotely()
{
    // Pass the URIs to the active Mudlet Server
    // The Mudlet Server may be owned by this process or another process.
    QLocalSocket socket;
    socket.connectToServer(mServerName);

    if (socket.waitForConnected(WAIT_FOR_RESPONSE_MS)) {
        const QString packagePathsData = mQueuedUris.join(QChar::LineFeed);
        socket.write(packagePathsData.toUtf8());
        socket.waitForBytesWritten(WAIT_FOR_RESPONSE_MS);
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

void MudletInstanceCoordinator::incomingConnection(quintptr socketDescriptor)
{
    QLocalSocket* socket = new QLocalSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QLocalSocket::readyRead, this, &MudletInstanceCoordinator::handleReadyRead);
    connect(socket, &QLocalSocket::disconnected, this, &MudletInstanceCoordinator::handleDisconnected);
}

// Receive URIs and open them
void MudletInstanceCoordinator::handleReadyRead()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    mMutex.lock();
    // Receive package paths and add them the install queue.
    QByteArray data = socket->readAll();
    const QStringList packagePaths = QString::fromUtf8(data).split(QChar::LineFeed, Qt::SkipEmptyParts);
    mQueuedUris << packagePaths;
    mMutex.unlock();

    openUrisLocally();
}

// Try to open queued URIs
// This is called when:
// - after a profile has been opened
// - after another instance of Mudlet has transmitted a list of URIs to this instance
void MudletInstanceCoordinator::openUrisLocally()
{
    mMutex.lock();
    QTimer::singleShot(0, this, [this]() {
        mudlet* mudletApp = mudlet::self();
        Q_ASSERT(mudletApp);

        // Process queued URIs until we have to wait for a profile to be selected or loaded
        for (qsizetype i = mQueuedUris.length() - 1; i >= 0; i--) {
            QUrl url = QUrl(mQueuedUris[i]);
            const bool isTelnet = url.scheme() == qsl("telnet") || url.scheme() == qsl("mudlet");
            const bool isPackage = url.scheme() == qsl("file");
            if (isTelnet) {
                // Telnet URI is found, so we need to handle it and open a profile.
                // Progress on uri queue will resume after the profile has been opened.
                mQueuedUris.removeLast();
                mMutex.unlock();
                mudletApp->handleTelnetUri(url);
                break;
            }
            if (isPackage) {
                Host* activeHost = mudletApp->getActiveHost();
                if (!activeHost) {
                    // Ask the user to choose a profile, since none are active.
                    // Progress on uri queue will resume after the profile has been opened.
                    mudletApp->slot_showConnectionDialog();
                    mMutex.unlock();
                    break;
                }
                // Install the package to the active host
                // Keep our lock on the mutex since the loop continues
                mQueuedUris.removeLast();
                activeHost->installPackage(url.toLocalFile(), 0);

                // If this loop won't run again, unlock the mutex
                if (i == 0) {
                    mMutex.unlock();
                }
            }
        }
    });
}


QStringList MudletInstanceCoordinator::readUriQueue()
{
    return mQueuedUris;
}

void MudletInstanceCoordinator::handleDisconnected()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
}
