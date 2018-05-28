#include "discord.h"

#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_register.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_rpc.h"

Discord::Discord(QObject *parent) : QObject(parent)
{
    qDebug() << "aww yeah";
}
