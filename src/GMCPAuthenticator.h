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

    void saveSupportsSet(const QString& packageMessage, const QString& data);
    void sendCredentials();
    void handleAuthResult(const QString& packageMessage, const QString& data);
    void handleAuthGMCP(const QString& packageMessage, const QString& data);

private:
    void sendOAuth(const QString& provider);
    void promptForOAuthProvider();
    void handleAuthUrl(const QString& packageMessage, const QString& data);
    void selectAuthMethod();
    void attemptReconnect();
    void sendReconnect(const QString& account, const QString& token);
    void handleAuthToken(const QString& packageMessage, const QString& data);
    void storeReconnectToken(const QString& account, const QString& token);
    void discardReconnectToken();

    Host* mpHost;
    QStringList mSupportedAuthTypes;
    QStringList mOAuthProviders;
    // True once the user opts into "remember me" for a fresh sign-in, or once a saved token is loaded
    // at connect time (so a rotated token from the server overwrites the stored one).
    bool mTokenPersistEnabled = false;
    // True while we are waiting for the Char.Login.Result that answers a Char.Login.Reconnect attempt,
    // so a failure can discard the stale token and fall back instead of aborting the login.
    bool mAwaitingReconnectResult = false;
    // Set when a reconnect token is rejected and we reconnect for a clean sign-in. The saved token is
    // cleared asynchronously, so this makes the next connection skip it explicitly rather than racing
    // the keychain removal and looping back into another rejected reconnect.
    bool mReconnectRejected = false;
};

#endif // MUDLET_AUTHENTICATOR_H
