#ifndef DISCORD_H
#define DISCORD_H

#include "pre_guard.h"
#include <QObject>
#include <QDebug>
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_register.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_rpc.h"
#include "post_guard.h"

class Discord : public QObject
{
    Q_OBJECT

public:
    explicit Discord(QObject *parent = nullptr);    
    ~Discord() override;

private:
    static void handleDiscordReady(const DiscordUser* request);
    static void handleDiscordDisconnected(int errorCode, const char* message);
    static void handleDiscordError(int errorCode, const char* message);
    static void handleDiscordJoinGame(const char* joinSecret);
    static void handleDiscordSpectateGame(const char* spectateSecret);
    static void handleDiscordJoinRequest(const DiscordUser* request);

signals:

public slots:
};

#endif // DISCORD_H
