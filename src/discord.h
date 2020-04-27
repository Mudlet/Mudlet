#ifndef DISCORD_H
#define DISCORD_H
/***************************************************************************
 *   Copyright (C) 2018 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2018 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "Host.h"

#include "pre_guard.h"
#include <functional>
#include <utility>
#include <QDebug>
#include <QReadWriteLock>
#include <QTimer>
#include <QTimerEvent>
#include <QLibrary>
#include "../3rdparty/discord/rpc/include/discord_register.h"
#include "../3rdparty/discord/rpc/include/discord_rpc.h"
#include "post_guard.h"

/*
 * From the discord headers and on-line documentation:
 * typedef struct DiscordRichPresence {
 *    const char* state; // max 128 bytes
 *    const char* details; // max 128 bytes
 *    int64_t startTimestamp;
 *    int64_t endTimestamp;
 *    const char* largeImageKey; // max 32 bytes
 *    const char* largeImageText; // max 128 bytes
 *    const char* smallImageKey; // max 32 bytes
 *    const char* smallImageText; // max 128 bytes
 *    const char* partyId; // max 128 bytes
 *    int partySize;
 *    int partyMax;
 *    const char* matchSecret; // max 128 bytes
 *    const char* joinSecret; // max 128 bytes
 *    const char* spectateSecret; // max 128 bytes
 *    int8_t instance;
 * } DiscordRichPresence;
 *
 *
 * typedef struct DiscordUser {
 *    const char* userId; // max 32 bytes
 *    const char* username; // max 344 bytes
 *    const char* discriminator; // max 8 bytes
 *    const char* avatar; // max 128 bytes
 * } DiscordUser;
 */

// This is used to hold data to be stuffed into a DiscordRichPresence before
// it is sent to the RPC library with an Discord_UpdatePresence(...) call.
// It is done this way because the definition we have for the
// DiscordRichPresence is filled with const char pointers that can only be
// set on instantiation.
class localDiscordPresence {

public:
    localDiscordPresence()
    : mState(), mDetails()
    , mStartTimestamp(0), mEndTimestamp(0)
    , mLargeImageKey(), mLargeImageText()
    , mSmallImageKey(), mSmallImageText()
    , mPartyId(), mPartySize(0), mPartyMax(0)
    , mMatchSecret(), mJoinSecret(), mSpectateSecret()
    , mInstance(1)
    {
    }

    void setStateText(const QString&);
    void setDetailText(const QString&);
    void setStartTimeStamp(int64_t startTime) { mStartTimestamp = startTime; }
    void setEndTimeStamp(int64_t endTime) { mEndTimestamp = endTime; }
    void setLargeImageKey(const QString&);
    void setLargeImageText(const QString&);
    void setSmallImageKey(const QString&);
    void setSmallImageText(const QString&);
    void setJoinSecret(const QString&);
    void setMatchSecret(const QString&);
    void setSpectateSecret(const QString&);
    void setPartySize(const int n) { mPartySize = n; }
    void setPartyMax(const int n) { mPartyMax = n; }
    DiscordRichPresence convert() const;
    QString getStateText() const { return QString::fromUtf8(mState); }
    QString getDetailText() const { return QString::fromUtf8(mDetails); }
    int64_t getStartTimeStamp() const { return mStartTimestamp; }
    int64_t getEndTimeStamp() const { return mEndTimestamp; }
    QString getLargeImageKey() const { return QString::fromUtf8(mLargeImageKey); }
    QString getLargeImageText() const { return QString::fromUtf8(mLargeImageText); }
    QString getSmallImageKey() const { return QString::fromUtf8(mSmallImageKey); }
    QString getSmallImageText() const { return QString::fromUtf8(mSmallImageText); }
    QString getJoinSecret() const { return QString::fromUtf8(mJoinSecret); }
    QString getMatchSecret() const { return QString::fromUtf8(mMatchSecret); }
    QString getSpectateSecret() const { return QString::fromUtf8(mSpectateSecret); }
    QString getPartyId() const { return QString::fromUtf8(mPartyId); }
    int getPartySize() const { return mPartySize; }
    int getPartyMax() const { return mPartyMax; }
    int8_t getInstance() const { return mInstance; }

private:
    char mState[128];
    char mDetails[128];
    int64_t mStartTimestamp;
    int64_t mEndTimestamp;
    char mLargeImageKey[32];
    char mLargeImageText[128];
    char mSmallImageKey[32];
    char mSmallImageText[128];
    char mPartyId[128];
    int mPartySize;
    int mPartyMax;
    char mMatchSecret[128];
    char mJoinSecret[128];
    char mSpectateSecret[128];
    int8_t mInstance;
};

#ifndef QT_NO_DEBUG_STREAM
// Note "inline" is REQUIRED:
inline QDebug& operator<<(QDebug& debug, const localDiscordPresence& ldp)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver);

    QString result = QStringLiteral("localDiscordPresence(\n"
                                    "    mDetails: \"%1\"  mState: \"%2\" mInstance: %3\n"
                                    "    mLargeImageKey: \"%4\"  mLargeImageText: \"%5\" \n"
                                    "    mSmallImageKey: \"%6\"  mSmallImageText: \"%7\" \n")
                     .arg(ldp.getDetailText(), ldp.getStateText(),
                          QString::number(ldp.getInstance()),
                          ldp.getLargeImageKey(), ldp.getLargeImageText(),
                          ldp.getSmallImageKey(), ldp.getSmallImageText());

    result.append(QStringLiteral("    mPartyId: \"%1\"  mPartySize: %2 mPartyMax %3\n"
                                 "    mMatchSecret: \"%4\"  mJoinSecret: \"%5\" mSpectateSecret \"%6\"\n"
                                 "    mStartTimeStamp: %7  mEndTimeStamp: %8)\n")
                  .arg(ldp.getPartyId(), QString::number(ldp.getPartySize()), QString::number(ldp.getPartyMax()),
                       ldp.getMatchSecret(), ldp.getJoinSecret(), ldp.getSpectateSecret(),
                       QString::number(ldp.getStartTimeStamp()), QString::number(ldp.getEndTimeStamp())));

    debug.nospace().noquote() << result;
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

class Discord : public QObject
{
    Q_OBJECT

public:
    explicit Discord(QObject *parent = nullptr);
    ~Discord() override;

    bool libraryLoaded();
    bool usingMudletsDiscordID(Host*) const;

    void UpdatePresence();

    QString deduceGameName(const QString& address);
    QPair<bool, QString> gameIntegrationSupported(const QString& address);

    void setLargeImage(Host*, const QString&);
    void setLargeImageText(Host*, const QString&);
    void setSmallImage(Host*, const QString&);
    void setSmallImageText(Host*, const QString&);
    void setStateText(Host*, const QString&);
    void setDetailText(Host*, const QString&);
    void setStartTimeStamp(Host*, int64_t);
    void setEndTimeStamp(Host*, int64_t);
    void setParty(Host*, int);
    void setParty(Host*, int, int);
    bool setApplicationID(Host*, const QString&);
    QString getApplicationId(Host* pHost) const;

    // These retrieve the cached data:
    QString getDetailText(Host* pHost) const { return mDetailTexts.value(pHost); }
    QString getStateText(Host* pHost) const { return mStateTexts.value(pHost); }
    QString getLargeImage(Host* pHost) const { return mLargeImages.value(pHost); }
    QString getLargeImageText(Host* pHost) const { return mLargeImageTexts.value(pHost); }
    QString getSmallImage(Host* pHost) const { return mSmallImages.value(pHost); }
    QString getSmallImageText(Host* pHost) const { return mSmallImageTexts.value(pHost); }
    QPair<int64_t ,int64_t> getTimeStamps(Host* pHost) const { return qMakePair(mStartTimes.value(pHost), mEndTimes.value(pHost)); }
    QPair<int, int> getParty(Host* pHost) const { return qMakePair(mPartySize.value(pHost), mPartyMax.value(pHost)); }

    // Returns the Discord user received from the Discord_Ready callback
    QStringList getDiscordUserDetails() const;

    // Runs the Host::discordUserIdMatch(...) check for the given Host:
    bool discordUserIdMatch(Host* pHost) const;


    const static QString mMudletApplicationId;


private:
    static void handleDiscordReady(const DiscordUser* request);
    static void handleDiscordDisconnected(int errorCode, const char* message);
    static void handleDiscordError(int errorCode, const char* message);
    static void handleDiscordJoinGame(const char* joinSecret);
    static void handleDiscordSpectateGame(const char* spectateSecret);
    static void handleDiscordJoinRequest(const DiscordUser* request);

    void timerEvent(QTimerEvent *event) override;

    DiscordEventHandlers* mpHandlers;

    // These are function pointers to functions located in the Discord RPC library:
    std::function<void(const char*, DiscordEventHandlers*, int, const char*)> Discord_Initialize;
    std::function<void(const DiscordRichPresence*)> Discord_UpdatePresence;
    std::function<void(void)> Discord_RunCallbacks;
    std::function<void(void)> Discord_Shutdown;
    // Not used:
    // std::function<void>(void)> Discord_ClearPresence;
#if defined(DISCORD_DISABLE_IO_THREAD)
    // std::function<void(void)> Discord_UpdateConnection;
#endif
    // std::function<void(const char*, int)> Discord_Respond;
    // std::function<void(DiscordEventHandlers*)> Discord_UpdateHandlers;

    bool mLoaded;

    // Key is a Application Id, Value is a pointer to a local copy of the data
    // currently held for that presence:
    QMap<QString, localDiscordPresence*> mPresencePtrs;

    // Used to tie a profile to a particular Discord presence - multiple
    // profiles can have the same presence but defaults to the nullptr one for
    // Mudlet:
    QMap<Host*, QString>mHostApplicationIDs;

    QScopedPointer<QLibrary> mpLibrary;

    // Used to hold the per profile data independently of whichever application id
    // it will be used with:
    QMap<Host*, int64_t>mStartTimes;
    QMap<Host*, int64_t>mEndTimes;
    QMap<Host*, QString>mDetailTexts;
    QMap<Host*, QString>mStateTexts;
    QMap<Host*, QString>mLargeImages;
    QMap<Host*, QString>mLargeImageTexts;
    QMap<Host*, QString>mSmallImages;
    QMap<Host*, QString>mSmallImageTexts;
    QMap<Host*, int>mPartySize;
    QMap<Host*, int>mPartyMax;

    // Hash with game name as key and various URL forms that might be used for
    // it as values:
    QHash<QString, QVector<QString>> mKnownGames;

    // The application ID that is currently the one that the Discord RPC library
    // has been set to use - only ONE can be active at a time currently,
    // though an Issue does exist to revise that at Discord:
    // https://github.com/discordapp/discord-rpc/issues/202
    QString mCurrentApplicationId;

    // Protect the four values after this one from async processes:
    static QReadWriteLock smReadWriteLock;
    // These are needed to validate the local user's presence on Discord to
    // the one that they want to be associated with a profile's character name
    // - it may be desired to not reveal the character name on Discord until
    // that has confirmed that a currently active Discord/Discord-PTB/
    // Discord-Canary application is using the expected User identity (reflected
    // in the User Avatar image and name within that application).
    static QString smUserName;
    static QString smUserId;
    static QString smDiscriminator;
    static QString smAvatar;
};

#endif // DISCORD_H
