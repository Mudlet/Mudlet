/***************************************************************************
 *   Copyright (C) 2023-2024 by Adam Robinson - seldon1951@hotmail.com     *
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

#ifndef MUDLETINSTANCECOORDINATOR_H
#define MUDLETINSTANCECOORDINATOR_H

#include "Host.h"
#include <QHash>
#include <QLocalServer>
#include <QMutex>
#include <QStringList>

class MudletInstanceCoordinator : public QLocalServer
{
    Q_OBJECT

public:
    explicit MudletInstanceCoordinator(const QString& serverName, QObject* parent = nullptr);
    bool tryToStart();
    void queuePackage(const QString& packageName);
    void installPackagesToHost(Host* activeProfile);
    void installPackagesLocally();
    bool installPackagesRemotely();
    QStringList readPackageQueue();

    void queueTelnetUri(const QString& uri);
    bool forwardTelnetUriToRunningInstance();

protected:
    void incomingConnection(quintptr socketDescriptor) override;

private slots:
    void handleReadyRead();
    void handleDisconnected();

private:
    QMutex mMutex;
    QString mServerName;
    QStringList mQueuedPackagePaths;
    QString mQueuedTelnetUri;
    QHash<QLocalSocket*, QByteArray> mSocketBuffers;
};

#endif // MUDLETINSTANCECOORDINATOR_H
