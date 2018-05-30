#ifndef DISCORD_H
#define DISCORD_H

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

    bool setGame(const QString& name);
    void setStatus(const QString& status);
    bool setSmallIcon(const QString& icon);
    bool setSmallIconText(const QString& icon);

private:
    std::function<void(const char*, DiscordEventHandlers*, int, const char*)> Discord_Initialize;
    std::function<void(const DiscordRichPresence* presence)> Discord_UpdatePresence;
    std::function<void(void)> Discord_RunCallbacks;
    std::function<void(void)> Discord_Shutdown;

    QString mGameName;
    QString mStatus;
    QString mSmallIcon;
    QString mSmallIconText;
    QScopedPointer<QLibrary> mpLibrary;
    bool mLoaded;

    static void handleDiscordReady(const DiscordUser* request);
    static void handleDiscordDisconnected(int errorCode, const char* message);
    static void handleDiscordError(int errorCode, const char* message);
    static void handleDiscordJoinGame(const char* joinSecret);
    static void handleDiscordSpectateGame(const char* spectateSecret);
    static void handleDiscordJoinRequest(const DiscordUser* request);
    void UpdatePresence();

    void timerEvent(QTimerEvent *event) override;

signals:

public slots:

};

#endif // DISCORD_H
