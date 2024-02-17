#include "GMCPAuthenticator.h"

#include <QDebug>

GMCPAuthenticator::GMCPAuthenticator(Host* pHost) : mpHost(pHost) {}

void GMCPAuthenticator::saveSupportsSet(const QString& message)
{
    auto jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    auto jsonObj = jsonDoc.object();

    if (jsonObj.contains("types")) {
        QJsonArray typesArray = jsonObj["types"].toArray();
        for (const auto& type : typesArray) {
            mSupportedAuthTypes.append(type.toString());
        }
    }

    qDebug() << "Supported auth types:" << mSupportedAuthTypes;
}

void GMCPAuthenticator::sendCredentials(const QString& character, const QString& password)
{
    auto character = mpHost->getLogin();
    auto password = mpHost->getPass();
    if (character.isEmpty() || password.isEmpty()) {
        qDebug() << "No login or password set in connection settings. Cannot authenticate with GMCP.";
        return;
    }

    QJsonObject credentials;
    credentials["character"] = character;
    credentials["password"] = password;

    QJsonDocument doc(credentials);
    QString gmcpMessage = doc.toJson(QJsonDocument::Compact);

    QString output = TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += "Client.Authenticate.Credentials";
    output += gmcpMessage;
    output += TN_IAC;
    output += TN_SE;

    // Send credentials to server
    socketOutRaw(output);
}


void GMCPAuthenticator::handleAuthResult(const QVariantMap& result)
{
    auto doc = QJsonDocument::fromJson(data.toUtf8());
    auto obj = doc.object();

    bool success = obj["success"].toBool();
    auto message = obj["message"].toString();

    if (success) {
        qDebug() << "GMCP login successful"
    } else {
        qDebug() << "GMCP login failed:" << message;
    }
}

// controller for GMCP authentication
void GMCPAuthenticator::handleAuthGMCP(const QString& packageMessage, const QString& data)
{
    if (packageMessage == qsl("Client.Authenticate.Default")) {
        saveSupportsSet(data);

        if (mSupportedAuthTypes.contains("credentials")) {
            sendCredentials("username", "password");
        } else {
            qDebug() << "Server does not support credentials authentication and we don't support any other";
        }
    }

    else if (packageMessage == qsl("Client.Authenticate.Result")) {
        handleAuthResult(data);

    } else {
        qDebug() << "Unknown GMCP auth package:" << packageMessage;
    }
}
