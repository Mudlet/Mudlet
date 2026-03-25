/***************************************************************************
 *   Copyright (C) 2018 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2018-2019, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "discord.h"
#include "mudlet.h"
#include "utils.h"

#include <QtDebug>
#include <QHash>

// Uncomment this to provide some additional qDebug() output:
// #define DEBUG_DISCORD 1

QString Discord::smUserName;
QString Discord::smUserId;
QString Discord::smDiscriminator;
QString Discord::smAvatar;
const QString Discord::mMudletApplicationId = qsl("450571881909583884");

Discord::Discord(QObject* parent)
: QObject(parent)
// For details see https://discord.com/developers/docs/rich-presence/how-to#initialization
// Initialise with a nullptr one with Mudlet's own ID
// NB: for testing the following MUDs have registered:
// "midmud"  is "460618737712889858", has "server-icon", "exventure" and "mudlet" icons
// "carinus" is "438335628942376960", has "server-icon" and "mudlet" icons
// "wotmud"  is "464945517156106240", has "mudlet", "ajar_(red|green|yellow|blue|white|grey|brown)"
, mHostApplicationIDs{{nullptr, mMudletApplicationId}} // lowercase list of known games
// {game name, {game addresses}}
, mKnownGames{
          {"midmud", {"midmud.com"}},
          {"wotmud", {"game.wotmud.org"}},
          {"luminari", {"luminarimud.com"}},
          {"achaea", {"achaea.com", "iron-ach.ironrealms.com"}},
          {"aetolia", {"aetolia.com", "iron-aet.ironrealms.com"}},
          {"imperian", {"imperian.com", "iron-imp.ironrealms.com"}},
          {"lusternia", {"lusternia.com", "iron-lus.ironrealms.com"}},
          {"starmourn", {"starmourn.com"}},
          {"stickmud", {"stickmud.com"}},
          {"clessidra", {"clessidra.it", "mud.clessidra.it"}},
          {"mume", {"mume.org"}},
          {"asteria", {"asteriamud.com"}},
  }
{
#if defined(Q_OS_WIN64)
    // Only defined on 64 bit Windows
    mpLibrary.reset(new QLibrary(qsl("discord-rpc64")));
#elif defined(Q_OS_WINDOWS)
    // Defined on both 32 and 64 bit Windows
    mpLibrary.reset(new QLibrary(qsl("discord-rpc32")));
#else
    // All other OSes
    mpLibrary.reset(new QLibrary(qsl("discord-rpc")));
#endif

    using Discord_InitializePrototype = void (*)(const char*, DiscordEventHandlers*, int, const char*);
    using Discord_UpdatePresencePrototype = void (*)(const DiscordRichPresence*);
    using Discord_RunCallbacksPrototype = void (*)();
    using Discord_ShutdownPrototype = void (*)();

    Discord_Initialize = reinterpret_cast<Discord_InitializePrototype>(mpLibrary->resolve("Discord_Initialize"));
    Discord_UpdatePresence = reinterpret_cast<Discord_UpdatePresencePrototype>(mpLibrary->resolve("Discord_UpdatePresence"));
    Discord_RunCallbacks = reinterpret_cast<Discord_RunCallbacksPrototype>(mpLibrary->resolve("Discord_RunCallbacks"));
    Discord_Shutdown = reinterpret_cast<Discord_ShutdownPrototype>(mpLibrary->resolve("Discord_Shutdown"));

    if (!mpLibrary->isLoaded() || !Discord_Initialize || !Discord_UpdatePresence || !Discord_RunCallbacks || !Discord_Shutdown) {
        const auto msg = mpLibrary->errorString();
        auto notFound = msg.contains(qsl("not found")) || msg.contains(qsl("No such file or directory"));
        qDebug().nospace() << "Could not " << (notFound ? "find" : "load") << " Discord library - searched in:";
        for (const auto& libraryPath : qApp->libraryPaths()) {
            qDebug() << "    " << libraryPath;
        }
        if (!msg.isEmpty() && !notFound) {
            qDebug().noquote().nospace() << "  error: \"" << msg << "\".";
        }
        return;
    }

    mLoaded = true;
    qDebug() << "Discord integration loaded. Using functions from:" << mpLibrary.data()->fileName();

    mpHandlers = new DiscordEventHandlers;
    memset(mpHandlers, 0, sizeof(DiscordEventHandlers));
    mpHandlers->ready = handleDiscordReady;
    mpHandlers->errored = handleDiscordError;
    mpHandlers->disconnected = handleDiscordDisconnected;
    mpHandlers->joinGame = handleDiscordJoinGame;
    mpHandlers->spectateGame = handleDiscordSpectateGame;
    mpHandlers->joinRequest = handleDiscordJoinRequest;

    // Initialize RPC eagerly so the IPC connection to Discord has time to
    // establish before the first UpdatePresence call
    initializeRpc();

    // mudlet instance is not available in this constructor as it's still being initialised, so postpone the connection
    QTimer::singleShot(0, this, [this]() {
        Q_ASSERT(mudlet::self());
        connect(mudlet::self(), &mudlet::signal_tabChanged, this, &Discord::UpdatePresence);

        // update Discord with the default Mudlet logo
        UpdatePresence();

        // process Discord callbacks every 50ms once we are all set up:
        startTimer(50);
    });
}

void Discord::initializeRpc()
{
    if (!mLoaded || mRpcActive) {
        return;
    }

    mCurrentApplicationId = mHostApplicationIDs.value(nullptr);
    Discord_Initialize(mCurrentApplicationId.toUtf8().constData(), mpHandlers, 0, nullptr);
    mRpcActive = true;
}

void Discord::shutdownRpc()
{
    if (!mRpcActive) {
        return;
    }

    Discord_Shutdown();
    mRpcActive = false;
    mCurrentApplicationId.clear();
}

Discord::~Discord()
{
    if (mRpcActive) {
        Discord_Shutdown();
        // We might expect to have to do an mpLibrary->unload() but we do not
        // need to as it happens automagically on the application shutdown...

        // Clear out the localDiscordPresence collection:
        QMutableMapIterator<QString, localDiscordPresence*> itPresencePtrs(mPresencePtrs);
        while (itPresencePtrs.hasNext()) {
            itPresencePtrs.next();
            delete itPresencePtrs.value();
            itPresencePtrs.remove();
        }
    }

    delete mpHandlers;
    mpHandlers = nullptr;
}

// For all the setters below the caller is supposed to check that they have the
// permission to do the operation
void Discord::setDetailText(Host* pHost, const QString& text)
{
    if (!text.isEmpty()) {
        mDetailTexts[pHost] = text;
    } else {
        mDetailTexts[pHost] = tr("via Mudlet");
    }

    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setStateText(Host* pHost, const QString& text)
{
    mStateTexts[pHost] = text;
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setLargeImage(Host* pHost, const QString& text)
{
    mLargeImages[pHost] = text;
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setLargeImageText(Host* pHost, const QString& text)
{
    mLargeImageTexts[pHost] = text;
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setSmallImage(Host* pHost, const QString& text)
{
    mSmallImages[pHost] = text;
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setSmallImageText(Host* pHost, const QString& text)
{
    mSmallImageTexts[pHost] = text;
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setStartTimeStamp(Host* pHost, int64_t epochTimeStamp)
{
    mStartTimes[pHost] = epochTimeStamp;
    mEndTimes.remove(pHost);
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setEndTimeStamp(Host* pHost, int64_t epochTimeStamp)
{
    mEndTimes[pHost] = epochTimeStamp;
    mStartTimes.remove(pHost);
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setParty(Host* pHost, int partySize)
{
    const int validPartySize = qMax(0, partySize);
    if (validPartySize) {
        // Is more than zero:
        if (mPartyMax.value(pHost) < validPartySize) {
            mPartyMax[pHost] = validPartySize;
        }

        mPartySize[pHost] = validPartySize;
    } else if (mPartyMax.contains(pHost)) {
        // There is a max size set - so zero this value
        mPartySize[pHost] = 0;
    } else {
        // There isn't a party size set so remove this (zero) value
        mPartySize.remove(pHost);
    }
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::setParty(Host* pHost, int partySize, int partyMax)
{
    const int validPartySize = qMax(0, partySize);
    const int validPartyMax = qMax(0, partyMax);

    if (validPartyMax) {
        // We have a party max size that is a positive number - so use the
        // largest of it and the size as the maximum:
        mPartyMax[pHost] = qMax(validPartySize, validPartyMax);
        mPartySize[pHost] = validPartySize;
    } else {
        // We have explicitly set the party maximum size to 0 (or less) - so
        // clear things:
        mPartySize.remove(pHost);
        mPartyMax.remove(pHost);
    }
    if (mLoaded) {
        UpdatePresence();
    }
}

void Discord::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)

    if (mLoaded && mRpcActive) {
        Discord_RunCallbacks();
    }
}

void Discord::handleDiscordReady(const DiscordUser* request)
{
    Discord::smUserName = request->username;
    Discord::smUserId = request->userId;
    Discord::smDiscriminator = request->discriminator;
    Discord::smAvatar = request->avatar;

#if defined(DEBUG_DISCORD)
    qDebug().noquote().nospace() << "Discord Ready callback received - for UserName: \"" << smUserName << "\", ID: \"" << smUserId << "#" << smDiscriminator << "\".";
#endif
    // don't call UpdatePresence from here - freezes Mudlet deep in the Discord API
    // when profile autostart is enabled
}

QStringList Discord::getDiscordUserDetails() const
{
    QStringList results;
    results << Discord::smUserName << Discord::smUserId << Discord::smDiscriminator << Discord::smAvatar;
    results.detach();
    return results;
}

void Discord::handleDiscordDisconnected(int errorCode, const char* message)
{
    qWarning() << "Discord disconnected - code:" << errorCode << "message:" << message;
}

void Discord::handleDiscordError(int errorCode, const char* message)
{
    qWarning() << "Discord error - code:" << errorCode << "message:" << message;
}

void Discord::handleDiscordJoinGame(const char* joinSecret)
{
    qDebug() << "Discord JoinGame received with secret:" << joinSecret;
}

void Discord::handleDiscordSpectateGame(const char* spectateSecret)
{
    qDebug() << "Discord SpectateGame received with secret:" << spectateSecret;
}

void Discord::handleDiscordJoinRequest(const DiscordUser* request)
{
    qDebug() << "Discord JoinRequest received from user:" << request->username << "userId:" << request->userId;
    qDebug() << "                         descriminator:" << request->discriminator << "avatar:" << request->avatar;
}

void Discord::UpdatePresence()
{
    if (!mLoaded) {
        return;
    }

    auto pHost = mudlet::self()->getActiveHost();

    // Don't send any presence when no profile is active or Discord is
    // disabled - showing "Playing Mudlet" would leak information when the
    // user hasn't opted in (see issue #6967)
    if (!pHost || pHost->mDiscordMode == Host::DiscordDisabled) {
        if (mRpcActive) {
            shutdownRpc();
        }
        return;
    }

    if (!mRpcActive) {
        initializeRpc();
    }

    if (pHost->mDiscordMode == Host::DiscordShowMudletOnly) {
        // Ensure we're using the default Mudlet application ID
        if (mCurrentApplicationId != mHostApplicationIDs.value(nullptr)) {
            shutdownRpc();
            initializeRpc();
        }
    }

    if (!pHost->discordUserIdMatch(Discord::smUserName, Discord::smDiscriminator)) {
#if defined(DEBUG_DISCORD)
        qDebug().nospace().noquote() << "Discord::UpdatePresence() INFO - Discord UserName/Discriminator does not match, not sending this update!";
#endif
        return;
    }

    // Need to establish which presence to use - will be null if it has not been overridden:
    QString applicationID = mHostApplicationIDs.value(pHost);

    if (mPresencePtrs.isEmpty()) {
        // First time only - with no localDiscordPresence in collection,
        // must just create the default one:
        auto* pTempPresence = new localDiscordPresence;
        mPresencePtrs.insert(QString(), pTempPresence);
    }

    // If the localDiscordPresence applicationID is NOT present in the existing
    // QMap then this will return a nullptr:
    localDiscordPresence* pDiscordPresence = nullptr;
    if (applicationID.isEmpty()) {
        pDiscordPresence = mPresencePtrs.value(nullptr);
        // Reset the empty applicationID to the one that belongs to Mudlet:
        applicationID = mHostApplicationIDs.value(nullptr);

        Q_ASSERT_X(pDiscordPresence, "Discord", "no Discord presence available for Mudlets default presence");
    } else {
        pDiscordPresence = mPresencePtrs.value(applicationID);

        if (!pDiscordPresence) {
            pDiscordPresence = new localDiscordPresence;
            mPresencePtrs.insert(applicationID, pDiscordPresence);
        }
    }

    if (mCurrentApplicationId != applicationID) {
#if defined(DEBUG_DISCORD)
        qDebug().nospace().noquote() << "Discord::UpdatePresence() INFO - mCurrentApplicationId (\"" << mCurrentApplicationId << "\") does not match the one for this Host instance (\""
                                     << applicationID << "\"), restarting RPC library with the latter.";
#endif
        Discord_Shutdown();

        Discord_Initialize(applicationID.toUtf8().constData(), mpHandlers, 0, nullptr);
        mCurrentApplicationId = applicationID;
    }

    if (!pDiscordPresence) {
        qCritical().noquote() << "Discord::UpdatePresence() CRITICAL - pDiscordPresence is unexpectedly a nullptr, unable to proceed with this procedure, please report this to Mudlet Makers!";
        return;
    }

    // Helper to decide if a field should be shown. Server-origin fields are
    // subject to mode and privacy flags; Lua-origin fields always pass.
    const bool isShowGameDetails = (pHost->mDiscordMode == Host::DiscordShowGameDetails);
    const auto shouldShow = [&](int flag) -> bool {
        if (!isServerOrigin(pHost, flag)) {
            return true;
        }
        // Server-origin: only show in ShowGameDetails mode when the privacy flag allows it
        return isShowGameDetails && (pHost->mDiscordAccessFlags & flag);
    };

    if (shouldShow(Host::DiscordSetDetail)) {
        pDiscordPresence->setDetailText(mDetailTexts.value(pHost));
    } else {
        pDiscordPresence->setDetailText(QString());
    }

    if (shouldShow(Host::DiscordSetState)) {
        pDiscordPresence->setStateText(mStateTexts.value(pHost));
    } else {
        pDiscordPresence->setStateText(QString());
    }

    if (shouldShow(Host::DiscordSetLargeIcon)) {
        auto image = mLargeImages.value(pHost);
        if (image.isEmpty() && applicationID == mMudletApplicationId) {
            image = qsl("mudlet");
        }
        pDiscordPresence->setLargeImageKey(image);
    } else {
        pDiscordPresence->setLargeImageKey(QString());
    }

    if (shouldShow(Host::DiscordSetLargeIconText)) {
        pDiscordPresence->setLargeImageText(mLargeImageTexts.value(pHost));
    } else {
        pDiscordPresence->setLargeImageText(QString());
    }

    if (shouldShow(Host::DiscordSetSmallIcon)) {
        pDiscordPresence->setSmallImageKey(mSmallImages.value(pHost));
    } else {
        pDiscordPresence->setSmallImageKey(QString());
    }

    if (shouldShow(Host::DiscordSetSmallIconText)) {
        pDiscordPresence->setSmallImageText(mSmallImageTexts.value(pHost));
    } else {
        pDiscordPresence->setSmallImageText(QString());
    }

    if (shouldShow(Host::DiscordSetPartyInfo) && mPartyMax.value(pHost)) {
        pDiscordPresence->setPartySize(mPartySize.value(pHost));
        pDiscordPresence->setPartyMax(mPartyMax.value(pHost));
    } else {
        pDiscordPresence->setPartySize(0);
        pDiscordPresence->setPartyMax(0);
    }

    if (shouldShow(Host::DiscordSetTimeInfo)) {
        if (mEndTimes.value(pHost)) {
            pDiscordPresence->setEndTimeStamp(mEndTimes.value(pHost));
            pDiscordPresence->setStartTimeStamp(0);
        } else {
            pDiscordPresence->setEndTimeStamp(0);
            pDiscordPresence->setStartTimeStamp(mStartTimes.value(pHost, 0));
        }
    } else {
        pDiscordPresence->setEndTimeStamp(0);
        pDiscordPresence->setStartTimeStamp(0);
    }

#if defined(DEBUG_DISCORD)
    qDebug().nospace().noquote() << "Discord::UpdatePresence() INFO - sending update:\n" << *pDiscordPresence;
#endif
    DiscordRichPresence const convertedPresence(pDiscordPresence->convert());
    Discord_UpdatePresence(&convertedPresence);
}

QString Discord::deduceGameName(const QString& address)
{
    // Handle using localhost as an off-line testing case
    if (address == QLatin1String("localhost") || address == QLatin1String("127.0.0.1") || address == QLatin1String("::1")) {
        return qsl("localhost");
    }

    // Handle the cases where the server url contains the "well-known" Server
    // name - that being the key of the QHash mKnownGames:
    if (mKnownGames.contains(address)) {
        return address;
    }

    // Do a bit of URL processing on the (potentially) host url:
    QString otherName;
    switch (address.count(QChar('.'))) {
    default:
        // Too complex - abandon
        qDebug().noquote().noquote() << "Discord::deduceGameName(\"" << address << "\") WARN - Unable to deduce MUD name from given address.";
        break;
    case 2: {
        // three terms - assume last is a TLD so remove it but the first may be significant

        QStringList fragments = address.split(QChar('.'));
        fragments.removeLast();
        otherName = fragments.join(QLatin1String("."));
        if (otherName.startsWith(QLatin1String("game."))) {
            // WoTMUD type case - so take remaining term in the middle of original
            otherName = otherName.split(QChar('.')).last();
            break;
        } else if (otherName.startsWith(QLatin1String("www."))) {
            // Error(?) in entering details so that a web-server name was give:
            otherName = otherName.split(QChar('.')).last();
            break;
        }
    }
        otherName.clear();
        break;
    case 1:
        // two terms - assume last is a TLD so remove it
        otherName = address.split(QChar('.')).first();
        break;
    case 0:
        // single term no need to split it
        otherName = address;
        break;
    }

    if (address.endsWith(qsl(".com"))) {
        otherName = address.left(address.length() - 4);
    } else if (address.endsWith(qsl(".de"))) {
        // Handle avalon.de case
        otherName = address.left(address.length() - 4);
    }

    // Handle the remaining cases where the known URL is something else - like
    // say a fixed IP address stored as a member of the value for the QHash
    // mKnownGames:
    QHashIterator<QString, QVector<QString>> itServer(mKnownGames);
    while (itServer.hasNext()) {
        itServer.next();
        QVectorIterator<QString> itUrl(itServer.value());
        while (itUrl.hasNext()) {
            if (itUrl.next().contains(address)) {
                return itServer.key();
            }
        }
    }

    // This may be an empty string but it is the best guess otherwise:
    return otherName;
}

// Returns true in First if this is a MUD we know about (and have an Icon for in
// on the Mudlet Discord server!) and the deduced name in Second - if the
// first is true.
QPair<bool, QString> Discord::gameIntegrationSupported(const QString& address)
{
    const QString deducedName = deduceGameName(address);

    // Handle using localhost as an off-line testing case
    if (deducedName == QLatin1String("localhost")) {
        return qMakePair(true, deducedName);
    }
    return qMakePair((!deducedName.isEmpty() && mKnownGames.contains(deducedName)), deducedName);
}

bool Discord::libraryLoaded()
{
    return mLoaded;
}

// AFAICT A Discord Application Id is an unsigned long long int (a.k.a. a
// quint64, or qulonglong)
bool Discord::setApplicationID(Host* pHost, const QString& text)
{
    const QString oldID = mHostApplicationIDs.value(pHost);
    if (oldID == text) {
        // No change so do nothing
        return true;
    }

    // Note what the current app ID is for the given Host - will be an empty
    // string if not overridden from the default Mudlet one:
    if (text.isEmpty()) {
        // An empty or null string is the signal to switch back to default
        // "Mudlet" presence - and always succeeds
        mHostApplicationIDs.remove(pHost);
        pHost->setDiscordApplicationID(QString());
        UpdatePresence();

        return true;
    }

    bool ok = false;
    if (text.toLongLong(&ok) && ok) {
        // Got something that makes a non-zero number - so assume it is ok
        mHostApplicationIDs[pHost] = text;
        pHost->setDiscordApplicationID(text);
        UpdatePresence();

        return true;
    }
    return false;
}

void Discord::resetData(Host* pHost)
{
    mStartTimes.remove(pHost);
    mEndTimes.remove(pHost);
    mDetailTexts[pHost] = qsl("www.mudlet.org");
    mStateTexts.remove(pHost);
    mLargeImages.remove(pHost);
    mLargeImageTexts.remove(pHost);
    mSmallImages.remove(pHost);
    mSmallImageTexts.remove(pHost);
    mPartySize.remove(pHost);
    mPartyMax.remove(pHost);
    mHostApplicationIDs.remove(pHost);
    mServerOriginFlags.remove(pHost);
    UpdatePresence();
}

void Discord::setServerOrigin(Host* pHost, int flag)
{
    mServerOriginFlags[pHost] |= flag;
}

void Discord::clearServerOrigin(Host* pHost, int flag)
{
    mServerOriginFlags[pHost] &= ~flag;
}

bool Discord::isServerOrigin(Host* pHost, int flag) const
{
    return mServerOriginFlags.value(pHost, 0) & flag;
}

// Returns Host set app ID or the default Mudlet one if none set for the
// specific Host:
QString Discord::getApplicationId(Host* pHost) const
{
    return mHostApplicationIDs.value(pHost, mHostApplicationIDs.value(nullptr));
}

DiscordRichPresence localDiscordPresence::convert() const
{
    // Discord RPC distinguishes between nullptr (field not set) and ""
    // (field set to empty). Pass nullptr for empty strings so Discord
    // hides the field rather than showing it as blank.
    const auto nullIfEmpty = [](const char* str) -> const char* {
        return (str && str[0] != '\0') ? str : nullptr;
    };

    return DiscordRichPresence{nullIfEmpty(mState),
                               nullIfEmpty(mDetails),
                               mStartTimestamp,
                               mEndTimestamp,
                               nullIfEmpty(mLargeImageKey),
                               nullIfEmpty(mLargeImageText),
                               nullIfEmpty(mSmallImageKey),
                               nullIfEmpty(mSmallImageText),
                               nullIfEmpty(mPartyId),
                               mPartySize,
                               mPartyMax,
                               nullIfEmpty(mMatchSecret),
                               nullIfEmpty(mJoinSecret),
                               nullIfEmpty(mSpectateSecret),
                               mInstance};
}

void localDiscordPresence::setDetailText(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mDetails, sizeof(mDetails), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setStateText(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mState, sizeof(mState), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setLargeImageText(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mLargeImageText, sizeof(mLargeImageText), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setLargeImageKey(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mLargeImageKey, sizeof(mLargeImageKey), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setSmallImageText(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mSmallImageText, sizeof(mSmallImageText), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setSmallImageKey(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mSmallImageKey, sizeof(mSmallImageKey), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setJoinSecret(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mJoinSecret, sizeof(mJoinSecret), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setMatchSecret(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mMatchSecret, sizeof(mMatchSecret), utf8Data.constData(), utf8Data.size());
}

void localDiscordPresence::setSpectateSecret(const QString& text)
{
    const QByteArray utf8Data = text.toUtf8();
    utils::copyString(mSpectateSecret, sizeof(mSpectateSecret), utf8Data.constData(), utf8Data.size());
}

bool Discord::usingMudletsDiscordID(Host* pHost) const
{
    return (!mHostApplicationIDs.contains(pHost));
}

bool Discord::discordUserIdMatch(Host* pHost) const
{
    return pHost->discordUserIdMatch(Discord::smUserName, Discord::smDiscriminator);
}
