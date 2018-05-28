#include "discord.h"

#include "pre_guard.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_register.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_rpc.h"
#include "post_guard.h"

using namespace std::chrono;

static const char* APPLICATION_ID = "450571881909583884";

Discord::Discord(QObject *parent) : QObject(parent)
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

    // start Discord's event loop
    startTimer(milliseconds(50));
}


Discord::~Discord()
{
    Discord_Shutdown();
}

void Discord::timerEvent(QTimerEvent *event)
{
    Discord_RunCallbacks();
}

void Discord::handleDiscordReady(const DiscordUser* request)
{
    qDebug() << "Discord handleDiscordReady!";
}

void Discord::handleDiscordDisconnected(int errorCode, const char* message)
{

    qDebug() << "Discord handleDiscordDisconnected!";
}

void Discord::handleDiscordError(int errorCode, const char* message)
{

    qDebug() << "Discord handleDiscordError!";
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
