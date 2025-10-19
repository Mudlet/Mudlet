#ifndef MUDLET_AUTHENTICATOR_H
#define MUDLET_AUTHENTICATOR_H

/***************************************************************************
 *   Copyright (C) 2024 by Vadim Peretokin - vperetokin@gmail.com          *
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
#include "utils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>


class GMCPAuthenticator
{
    Q_DECLARE_TR_FUNCTIONS(GMCPAuthenticator)

public:

    explicit GMCPAuthenticator(Host* pHost);
    ~GMCPAuthenticator() = default;

    void saveSupportsSet(const QString& data);
    void sendCredentials();
    void handleAuthResult(const QString& data);
    void handleAuthGMCP(const QString& packageMessage, const QString& data);

private:
    Host* mpHost;
    QStringList mSupportedAuthTypes;
};

#endif // MUDLET_AUTHENTICATOR_H
