#pragma once

#include "pre_guard.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>
#include "post_guard.h"

class Host;

#include "utils.h"

class GMCPAuthenticator : public QObject
{
    Q_OBJECT

public:
    GMCPAuthenticator(Host* pHost);

    void saveSupportsSet(const QString& message);

    void sendCredentials(const QString& character, const QString& password);

    void handleAuthResult(const QVariantMap& result);

    void handleAuthGMCP(const QString& packageMessage, const QString& data);

private:
    Host* mpHost;
    QStringList mSupportedAuthTypes;
};
