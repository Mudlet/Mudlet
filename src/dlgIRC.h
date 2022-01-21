#ifndef MUDLET_DLGIRC_H
#define MUDLET_DLGIRC_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "pre_guard.h"
#include "ui_irc.h"
#include <Irc>
#include <IrcBuffer>
#include <IrcBufferModel>
#include <IrcCommand>
#include <IrcCommandParser>
#include <IrcCompleter>
#include <IrcConnection>
#include <IrcMessage>
#include <IrcTextFormat>
#include <IrcUser>
#include <IrcUserModel>

#include <QPointer>
#include "post_guard.h"

#include "utils.h"

class Host;

class dlgIRC : public QMainWindow, public Ui::irc
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgIRC)
    explicit dlgIRC(Host*);
    ~dlgIRC();

    inline static QString HostNameCfgItem = qsl("irc_host");
    inline static QString HostPortCfgItem = qsl("irc_port");
    inline static QString HostSecureCfgItem = qsl("irc_secure");
    inline static QString NickNameCfgItem = qsl("irc_nick");
    inline static QString PasswordCfgItem = qsl("irc_password");
    inline static QString ChannelsCfgItem = qsl("irc_channels");
    inline static QString DefaultHostName = qsl("irc.libera.chat");
    inline static int DefaultHostPort = 6667;
    inline static bool DefaultHostSecure = false;
    inline static QString DefaultNickName = qsl("Mudlet");
    inline static QStringList DefaultChannels = QStringList() << qsl("Mudlet");
    inline static int DefaultMessageBufferLimit = 5000;


    static QString readIrcHostName(Host* pH);
    static int readIrcHostPort(Host* pH);
    static bool readIrcHostSecure(Host* pH);
    static QString readIrcNickName(Host* pH);
    static QString readIrcPassword(Host* pH);
    static QStringList readIrcChannels(Host* pH);
    static QPair<bool, QString> writeIrcHostName(Host* pH, const QString& hostname);
    static QPair<bool, QString> writeIrcHostPort(Host* pH, int port);
    static QPair<bool, QString> writeIrcHostSecure(Host* pH, bool secure);
    static QPair<bool, QString> writeIrcNickName(Host* pH, const QString& nickname);
    static QPair<bool, QString> writeIrcPassword(Host* pH, const QString& password);
    static QPair<bool, QString> writeIrcChannels(Host* pH, const QStringList& channels);

    IrcConnection* connection = nullptr;
    bool mReadyForSending = false;
    QPair<bool, QString> sendMsg(const QString& target, const QString& message);
    QString getHostName() const { return mHostName; }
    int getHostPort() const { return mHostPort; }
    bool getHostSecure() const { return mHostSecure; }
    QString getNickName() const { return mNickName; }
    QString getPassword() const { return mPassword; }
    QStringList getChannels() const { return mChannels; }
    QString getConnectedHost() const { return mConnectedHostName; }
    void ircRestart(bool reloadConfigs = true);

private slots:
    void slot_onConnected();
    void slot_onConnecting();
    void slot_onDisconnected();
    void slot_onTextEdited();
    void slot_onTextEntered();
    void slot_nameCompletion();
    void slot_nameCompleted(const QString& text, int cursor);
    void slot_onBufferAdded(IrcBuffer* buffer);
    void slot_onBufferRemoved(IrcBuffer* buffer);
    void slot_onBufferActivated(const QModelIndex& index);
    void slot_onUserActivated(const QModelIndex& index);
    void slot_nickNameRequired(const QString& reserved, QString* alt);
    void slot_nickNameChanged(const QString& nick);
    void slot_joinedChannel(IrcJoinMessage* message);
    void slot_partedChannel(IrcPartMessage* message);
    void slot_receiveMessage(IrcMessage* message);
    void slot_onAnchorClicked(const QUrl& link);
    void slot_onHistoryCompletion();
    void slot_receiveNumericMessage(IrcNumericMessage* msg);

private:
    void setClientWindowTitle();
    void startClient();
    void setupCommandParser();
    void setupBuffers();
    bool processCustomCommand(IrcCommand*);
    void displayHelp(const QString&);
    void appendHtml(QTextDocument*, const QString&);
    QString getMessageTarget(IrcMessage*, const QString&);
    static QString readAppDefaultIrcNick();
    static void writeAppDefaultIrcNick(const QString&);
    void writeQSettings();

    void showEvent(QShowEvent* event) override;

    Host* mpHost = nullptr;
    bool mIrcStarted = false;
    IrcCompleter* completer = nullptr;
    IrcCommandParser* commandParser = nullptr;
    IrcBufferModel* bufferModel = nullptr;
    QHash<IrcBuffer*, IrcUserModel*> userModels;
    QHash<IrcBuffer*, QTextDocument*> bufferTexts;
    QPointer<IrcBuffer> serverBuffer;
    QStringList mInputHistory;
    int mInputHistoryMax = 8;
    int mInputHistoryIdxNext = 0;
    int mInputHistoryIdxCurrent = 0;
    quint64 mPingStarted = 0;
    QString mConnectedHostName;
    QString mHostName;
    int mHostPort = 0;
    bool mHostSecure = false;
    QString mNickName;
    QString mUserName = qsl("mudlet");
    QString mPassword;
    QString mRealName;
    QStringList mChannels;
    int mMessageBufferLimit = 0;
};

#endif // MUDLET_DLGIRC_H
