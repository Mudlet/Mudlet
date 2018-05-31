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
  , mArea{}
  , mCharacterIcon{}
  , mCharacterText{}
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

    if (!Discord_Initialize || !Discord_UpdatePresence || !Discord_RunCallbacks || !Discord_Shutdown) {
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

bool Discord::setArea(const QString& area)
{
    if (mLoaded) {
        mArea = area;
        UpdatePresence();
        return true;
    }
    return false;
}

bool Discord::setCharacterIcon(const QString& icon)
{
    if (mLoaded) {
        mCharacterIcon = icon;
        UpdatePresence();
        return true;
    }
    return false;
}

bool Discord::setCharacterText(const QString& text)
{
    if (mLoaded) {
        mCharacterText = text;
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

    char buffer[256];
    buffer[0] = '\0';
    auto gameName = mGameName.toLower().toUtf8();
    auto area = mArea.toUtf8();
    auto smallIcon = mCharacterIcon.toLower().toUtf8();
    auto smallIconText = mCharacterText.toUtf8();
    if (!mGameName.isEmpty()) {
        sprintf(buffer, "Playing %s", mGameName.toUtf8().constData());
        discordPresence.details = buffer;
        discordPresence.largeImageKey = gameName.constData();
    }

    if (!area.isEmpty()) {
        discordPresence.state = area.constData();
    }

    if (!smallIcon.isEmpty()) {
        discordPresence.smallImageKey = smallIcon.constData();
    }

    if (!smallIconText.isEmpty()) {
        discordPresence.smallImageText = smallIconText.constData();
    }

    auto timestamp = static_cast<int64_t>(std::time(nullptr));
    discordPresence.startTimestamp = timestamp;

    discordPresence.instance = 1;
    Discord_UpdatePresence(&discordPresence);
}
