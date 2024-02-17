#include "GMCPAuthenticator.h"

#include <QDebug>

GMCPAuthenticator::GMCPAuthenticator()
{
    // Initialize
}

void GMCPAuthenticator::handleSupportsSet(const QString& message)
{
    // Parse message to get supported auth types
    // Store in mSupportedAuthTypes

    qDebug() << "Supported auth types:" << mSupportedAuthTypes;
}

void GMCPAuthenticator::sendCredentials(const QString& character, const QString& password)
{
    // Validate supported auth types includes credentials

    // Send Client.Authenticate.Credentials message

    qDebug() << "Sent credentials for" << character;
}

void GMCPAuthenticator::handleAuthResult(const QVariantMap& result)
{
    // Check result success and message
    // Take action as needed

    qDebug() << "Auth" << (result["success"].toBool() ? "succeeded" : "failed");
}

void GMCPAuthenticator::handleAuthGMCP(const QString& packageMessage, const QString& data)
{
    if (packageMessage == qsl("Client.Authenticate.Default")) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8());
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("types")) {
            QJsonArray typesArray = jsonObj["types"].toArray();
            for (const auto& type : typesArray) {
                mSupportedAuthTypes.append(type.toString());
            }
        }
    }

    else if (packageMessage == "Client.Authenticate.Result") {
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonObject obj = doc.object();

        bool success = obj["success"].toBool();
        QString message = obj["message"].toString();

        if (success) {
            // Login succeeded
        } else {
            // Login failed, handle message
        }
    } else if (packageMessage == "Client.Authenticate.OAuth") {
        // Handle OAuth data based on flow
    } else {
        // Unknown package, log warning
    }
}
