#include "discord.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <functional>
#include <utility>
#include "post_guard.h"


// Mudlet's applicationID on Discord: https://discordapp.com/developers/docs/rich-presence/how-to#initialization
static const char* APPLICATION_ID = "450571881909583884";

Discord::Discord(QObject* parent) : QObject(parent)
  , mGamesNames{}
  , mAreas{}
  , mCharacterIcons{}
  , mCharacters{}
  , mLoaded{}
  , mStartTime{}
{
    mpLibrary.reset(new QLibrary(QStringLiteral("discord-rpc")));

    using Discord_InitializePrototype = void (*)(const char*, DiscordEventHandlers*, int, const char*);
    using Discord_UpdatePresencePrototype = void (*)(const DiscordRichPresence*);
    using Discord_RunCallbacksPrototype = void (*)();
    using Discord_ShutdownPrototype = void (*)();

    Discord_Initialize = reinterpret_cast<Discord_InitializePrototype>(mpLibrary->resolve("Discord_Initialize"));
    Discord_UpdatePresence = reinterpret_cast<Discord_UpdatePresencePrototype>(mpLibrary->resolve("Discord_UpdatePresence"));
    Discord_RunCallbacks = reinterpret_cast<Discord_RunCallbacksPrototype>(mpLibrary->resolve("Discord_RunCallbacks"));
    Discord_Shutdown = reinterpret_cast<Discord_ShutdownPrototype>(mpLibrary->resolve("Discord_Shutdown"));

    if (!Discord_Initialize || !Discord_UpdatePresence || !Discord_RunCallbacks || !Discord_Shutdown) {
        qDebug() << "Discord integration failed to load.";
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

    mStartTime = static_cast<int64_t>(std::time(nullptr));

    // 1234 is an optional Steam ID - we're not in Steam yet, so this value is fake one
    Discord_Initialize(APPLICATION_ID, &handlers, 1, "1234");

    // mudlet instance is not available in this constructor as it's still being initialised, so postpone the connection
    QTimer::singleShot(0, [this]() {
        Q_ASSERT(mudlet::self());
        connect(mudlet::self(), &mudlet::signal_tabChanged, this, &Discord::UpdatePresence);
    });


    // process Discord callbacks every 50ms
    startTimer(50);
}


Discord::~Discord()
{
    if (mLoaded) {
        Discord_Shutdown();
    }
}

bool Discord::setGame(Host* pHost, const QString& name)
{
    if (mLoaded) {
        mGamesNames[pHost] = name;
        UpdatePresence();
        return true;
    }
    return false;
}

bool Discord::setArea(Host* pHost, const QString& area)
{
    if (mLoaded) {
        mAreas[pHost] = area;
        UpdatePresence();
        return true;
    }
    return false;
}

bool Discord::setCharacterIcon(Host* pHost, const QString& icon)
{
    if (mLoaded) {
        mCharacterIcons[pHost] = icon;
        UpdatePresence();
        return true;
    }
    return false;
}

bool Discord::setCharacter(Host* pHost, const QString& text)
{
    if (mLoaded) {
        mCharacters[pHost] = text;
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
    qWarning() << "Discord disconnected:" << errorCode << message;
}

void Discord::handleDiscordError(int errorCode, const char* message)
{
    qWarning() << "Discord error:" << errorCode << message;
}

void Discord::handleDiscordJoinGame(const char* joinSecret)
{
    Q_UNUSED(joinSecret);
}

void Discord::handleDiscordSpectateGame(const char* spectateSecret)
{
    Q_UNUSED(spectateSecret);
}

void Discord::handleDiscordJoinRequest(const DiscordUser* request)
{
    Q_UNUSED(request);
}

void Discord::UpdatePresence()
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    char buffer[256];
    buffer[0] = '\0';
    auto gameName = mGamesNames[mudlet::self()->getActiveHost()].toUtf8();
    auto gameNameLowercase = mGamesNames[mudlet::self()->getActiveHost()].toLower().toUtf8();
    auto area = mAreas[mudlet::self()->getActiveHost()].toUtf8();
    auto characterIcon = mCharacterIcons[mudlet::self()->getActiveHost()].toLower().toUtf8();
    auto characterText = mCharacters[mudlet::self()->getActiveHost()].toUtf8();

    if (!gameName.isEmpty()) {
        sprintf(buffer, "Playing %s", gameName.constData());
        discordPresence.details = buffer;
        discordPresence.largeImageKey = gameNameLowercase.constData();
    }

    if (!area.isEmpty()) {
        discordPresence.state = area.constData();
    }

    if (!characterIcon.isEmpty()) {
        discordPresence.smallImageKey = characterIcon.constData();
    }

    if (!characterText.isEmpty()) {
        discordPresence.smallImageText = characterText.constData();
    }

//    discordPresence.startTimestamp = mStartTime;

    discordPresence.instance = 1;
    Discord_UpdatePresence(&discordPresence);
}
