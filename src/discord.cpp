#include "discord.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <functional>
#include <utility>
#include "post_guard.h"


// Mudlet's applicationID on Discord: https://discordapp.com/developers/docs/rich-presence/how-to#initialization
static const char* APPLICATION_ID = "450571881909583884";

Discord::Discord(QObject* parent) : QObject(parent)
  , mGameName{}
  , mStatus{}
  , mSmallIcon{}
  , mSmallIconText{}
  , mLoaded{}
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
        qWarning() << "Discord integration is unavailable.";
        return;
    }

    mLoaded = true;
    qWarning() << "Discord integration loaded.";

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

bool Discord::setGame(const QString& name)
{
    if (mLoaded) {
        mGameName = name;
        UpdatePresence();
        return true;
    }

    return false;
}

void Discord::setStatus(const QString& status)
{
    if (mLoaded) {
        mStatus = status;
        UpdatePresence();
    }
}

bool Discord::setSmallIcon(const QString& icon)
{
    if (mLoaded) {
        mSmallIcon = icon;
        UpdatePresence();
        return true;
    }

    return false;
}

bool Discord::setSmallIconText(const QString& iconText)
{
    if (mLoaded) {
        mSmallIconText = iconText;
        UpdatePresence();
        return true;
    }

    return false;
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

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    if (!mGameName.isEmpty()) {
        char buffer[256];
        sprintf(buffer, "Playing %s", mGameName.toUtf8().constData());
        discordPresence.details = buffer;

        char largeImageKeyBuffer[32];
        auto gameName = mGameName.toLower().toUtf8();
        strncpy(largeImageKeyBuffer, gameName.constData(), gameName.length());
        discordPresence.largeImageKey = largeImageKeyBuffer;
    }

    if (!mStatus.isEmpty()) {
        char buffer[128];
        auto status = mStatus.toUtf8();
        strncpy(buffer, status.constData(), status.length());
        discordPresence.state = status.constData();
    }

    if (!mSmallIcon.isEmpty()) {
        char buffer[32];
        auto gameName = mSmallIcon.toLower().toUtf8();
        strncpy(buffer, gameName.constData(), gameName.length());
        discordPresence.smallImageKey = buffer;
    }

    if (!mSmallIconText.isEmpty()) {
        char buffer[128];
        auto smallIconText = mSmallIconText.toUtf8();
        strncpy(buffer, smallIconText.constData(), smallIconText.length());
        discordPresence.smallImageText = buffer;
    }

    auto timestamp = static_cast<int64_t>(std::time(nullptr));
    discordPresence.startTimestamp = timestamp;

    discordPresence.instance = 1;
    Discord_UpdatePresence(&discordPresence);
}
