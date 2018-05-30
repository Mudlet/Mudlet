#include "discord.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <functional>
#include "post_guard.h"


// Mudlet's applicationID on Discord: https://discordapp.com/developers/docs/rich-presence/how-to#initialization
static const char* APPLICATION_ID = "450571881909583884";

Discord::Discord(QObject* parent) : QObject(parent), mDiscordPresence(), mLoaded(false)
{
    mpLibrary.reset(new QLibrary(QStringLiteral("libdiscord-rpc")));

    using Discord_InitializePrototype = void (*)(const char*, DiscordEventHandlers*, int, const char*);
    using Discord_UpdatePresencePrototype = void (*)(const DiscordRichPresence*);
    using Discord_RunCallbacksPrototype = void (*)();
    using Discord_ShutdownPrototype = void (*)();

    Discord_Initialize = reinterpret_cast<Discord_InitializePrototype>(mpLibrary->resolve("Discord_Initialize"));
    Discord_UpdatePresence = reinterpret_cast<Discord_UpdatePresencePrototype>(mpLibrary->resolve("Discord_UpdatePresence"));
    Discord_RunCallbacks = reinterpret_cast<Discord_RunCallbacksPrototype>(mpLibrary->resolve("Discord_RunCallbacks"));
    Discord_Shutdown = reinterpret_cast<Discord_ShutdownPrototype>(mpLibrary->resolve("Discord_Shutdown"));

    if (!Discord_Initialize || !Discord_UpdatePresence || !Discord_RunCallbacks) {
        return;
    }

    mLoaded = true;
    qDebug() << "Discord integration loaded.";

    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.errored = handleDiscordError;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.joinGame = handleDiscordJoinGame;
    handlers.spectateGame = handleDiscordSpectateGame;
    handlers.joinRequest = handleDiscordJoinRequest;

    // 1234 is an optional Steam ID - we're not in Steam yet, so this value is fake one
    Discord_Initialize(APPLICATION_ID, &handlers, 1, "1234");

    // process Discord callbacks every 50ms
    startTimer(50);
}


Discord::~Discord()
{
    if (mLoaded) {
        Discord_Shutdown();
    }
}

void Discord::setGameName(const QString& name)
{
    if (!mLoaded) {
        return;
    }

    char buffer[256];
    sprintf(buffer, "Playing %s", name.toUtf8().constData());
    mDiscordPresence.details = buffer;
    mDiscordPresence.largeImageKey = name.toLower().toUtf8().constData();

    Discord_UpdatePresence(&mDiscordPresence);
}

void Discord::setStatus(const QString& status)
{
    if (!mLoaded) {
        return;
    }

    char buffer[256];
    sprintf(buffer, "%s", status.toUtf8().constData());
    mDiscordPresence.state = buffer;

    Discord_UpdatePresence(&mDiscordPresence);
}

void Discord::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event);

    if (mLoaded) {
        Discord_RunCallbacks();
    }
}

void Discord::handleDiscordReady(const DiscordUser* request)
{
    Q_UNUSED(request);

    mudlet::self()->mDiscord.UpdatePresence();
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
    memset(&mDiscordPresence, 0, sizeof(mDiscordPresence));
    //    mDiscordPresence.state = "In a raiding party";
    //    sprintf(buffer, "Playing Achaea");
    //    mDiscordPresence.details = buffer;
    //    mDiscordPresence.startTimestamp = time(0);
    ////    mDiscordPresence.smallImageKey = "achaea";
    //    mDiscordPresence.largeImageKey = "achaea";
    //    mDiscordPresence.largeImageText = "Dragon (level 100)";

    mDiscordPresence.state = "Raiding party";
    sprintf(buffer, "Playing Luminari");
    mDiscordPresence.details = buffer;
    mDiscordPresence.startTimestamp = time(0);
    //    mDiscordPresence.smallImageKey = "achaea";
    mDiscordPresence.largeImageKey = "luminari";
    mDiscordPresence.largeImageText = "Dragon (level 100)";

    mDiscordPresence.partySize = 5;
    mDiscordPresence.partyMax = 6;
    mDiscordPresence.matchSecret = "zdgfghrfsyheqrwqgshbfxdq35 4 5";
    mDiscordPresence.joinSecret = "ASFDFSHR512345 RASGSADWr";
    mDiscordPresence.instance = 1;
    Discord_UpdatePresence(&mDiscordPresence);
}
