#pragma once

#include <QString>
#include <QVariantMap>

class GMCPAuthenticator
{
public:
    GMCPAuthenticator();

    void handleSupportsSet(const QString& message);

    void sendCredentials(const QString& character, const QString& password);

    void handleAuthResult(const QVariantMap& result);

    void handleAuthGMCP(const QString& packageMessage, const QString& data);

private:
    QStringList mSupportedAuthTypes;
};
