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

#ifndef MUDLETSERVER_H
#define MUDLETSERVER_H

#include <QStringList>
#include <QLocalServer>
#include "Host.h"

class MudletServer : public QLocalServer
{
    Q_OBJECT

public:
    explicit MudletServer(const QString& serverName, QObject* parent = nullptr);
    bool tryToStart();
    bool tryInstall(const QString& packageName);
    void tryInstallQueuedPackages(Host* activeProfile);

protected:
    void incomingConnection(quintptr socketDescriptor) override;

private slots:
    void handleReadyRead();
    void handleDisconnected();

private:
    QString mServerName;
    QStringList mQueuedPackagePaths;
};

#endif // MUDLETSERVER_H
