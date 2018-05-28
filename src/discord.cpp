#include "discord.h"

#include "pre_guard.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_register.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_rpc.h"
#include "post_guard.h"

static const char* APPLICATION_ID = "450571881909583884";

Discord::Discord(QObject *parent) : QObject(parent)
, mDiscordPresence()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.errored = handleDiscordError;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.joinGame = handleDiscordJoinGame;
    handlers.spectateGame = handleDiscordSpectateGame;
    handlers.joinRequest = handleDiscordJoinRequest;

    Discord_Initialize(APPLICATION_ID, &handlers, 1, "1234");

    // start Discord's event loop at 50ms
    startTimer(50);
}


Discord::~Discord()
{
    Discord_Shutdown();
}

void Discord::setGameName(const QString& name)
{
    char buffer[256];
    sprintf(buffer, "Playing %s", name.toUtf8().constData());
    mDiscordPresence.details = buffer;
    mDiscordPresence.largeImageKey = name.toLower().toUtf8().constData();

    Discord_UpdatePresence(&mDiscordPresence);
}

void Discord::setStatus(const QString& status)
{
    char buffer[256];
    sprintf(buffer, "%s", status.toUtf8().constData());
    mDiscordPresence.state = buffer;

    Discord_UpdatePresence(&mDiscordPresence);
}

void Discord::timerEvent(QTimerEvent *event)
{
    Discord_RunCallbacks();
}

void Discord::handleDiscordReady(const DiscordUser* request)
{
    qDebug() << "Discord handleDiscordReady!";
    UpdatePresence();
}

void Discord::handleDiscordDisconnected(int errorCode, const char* message)
{

    qDebug() << "Discord handleDiscordDisconnected!";
}

void Discord::handleDiscordError(int errorCode, const char* message)
{
    qWarning() << "Discord error:" << errorCode << message;
}

void Discord::handleDiscordJoinGame(const char* joinSecret)
{

    qDebug() << "Discord handleDiscordJoinGame!";
}

void Discord::handleDiscordSpectateGame(const char* spectateSecret)
{

    qDebug() << "Discord handleDiscordSpectateGame!";
}

void Discord::handleDiscordJoinRequest(const DiscordUser* request)
{

    qDebug() << "Discord handleDiscordJoinRequest!";
}

void Discord::UpdatePresence()
{
    char buffer[256];
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
//    discordPresence.state = "In a raiding party";
//    sprintf(buffer, "Playing Achaea");
//    discordPresence.details = buffer;
//    discordPresence.startTimestamp = time(0);
////    discordPresence.smallImageKey = "achaea";
//    discordPresence.largeImageKey = "achaea";
//    discordPresence.largeImageText = "Dragon (level 100)";

    discordPresence.state = "Raiding party";
    sprintf(buffer, "Playing Luminari");
    discordPresence.details = buffer;
    discordPresence.startTimestamp = time(0);
//    discordPresence.smallImageKey = "achaea";
    discordPresence.largeImageKey = "luminari";
    discordPresence.largeImageText = "Dragon (level 100)";

    discordPresence.partySize = 5;
    discordPresence.partyMax = 6;
    discordPresence.matchSecret = "zdgfghrfsyheqrwqgshbfxdq35 4 5";
    discordPresence.joinSecret = "ASFDFSHR512345 RASGSADWr";
    discordPresence.instance = 1;
    Discord_UpdatePresence(&discordPresence);
}
