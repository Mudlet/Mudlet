#ifndef DISCORD_H
#define DISCORD_H

#include "Host.h"

#include "pre_guard.h"
#include <QDebug>
#include <QTimer>
#include <QTimerEvent>
#include <QLibrary>
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_register.h"
#include "../3rdparty/discord/discord-rpc-linux/discord-rpc/linux-dynamic/include/discord_rpc.h"
#include "post_guard.h"

class Discord : public QObject
{
    Q_OBJECT

public:
    explicit Discord(QObject *parent = nullptr);    
    ~Discord() override;

    void UpdatePresence();
    std::tuple<bool, QString> setGame(Host* pHost, const QString& name);
    bool setArea(Host* pHost, const QString& area);
    bool setCharacterIcon(Host* pHost, const QString& icon);
    bool setCharacter(Host* pHost, const QString& text);
    bool gameIntegrationSupported(const QString& address);
    bool libraryLoaded();

private:
    std::function<void(const char*, DiscordEventHandlers*, int, const char*)> Discord_Initialize;
    std::function<void(const DiscordRichPresence* presence)> Discord_UpdatePresence;
    std::function<void(void)> Discord_RunCallbacks;
    std::function<void(void)> Discord_Shutdown;

    QMap<Host*, QString>mGamesNames;
    QMap<Host*, QString>mAreas;
    QMap<Host*, QString>mCharacterIcons;
    QMap<Host*, QString>mCharacters;

    QScopedPointer<QLibrary> mpLibrary;
    bool mLoaded;
    int64_t mStartTime;

    QHash<QString, QVector<QString>> mKnownGames;
    QVector<QString> mKnownAddresses;

    static void handleDiscordReady(const DiscordUser* request);
    static void handleDiscordDisconnected(int errorCode, const char* message);
    static void handleDiscordError(int errorCode, const char* message);
    static void handleDiscordJoinGame(const char* joinSecret);
    static void handleDiscordSpectateGame(const char* spectateSecret);
    static void handleDiscordJoinRequest(const DiscordUser* request);

    void timerEvent(QTimerEvent *event) override;

signals:

public slots:

};

#endif // DISCORD_H
