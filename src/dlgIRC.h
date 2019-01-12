#ifndef MUDLET_DLGIRC_H
#define MUDLET_DLGIRC_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
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

class Host;

class dlgIRC : public QMainWindow, public Ui::irc
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgIRC)
    dlgIRC(Host*);
    ~dlgIRC();

    static QString HostNameCfgItem;
    static QString HostPortCfgItem;
    static QString NickNameCfgItem;
    static QString ChannelsCfgItem;
    static QString DefaultHostName;
    static int DefaultHostPort;
    static QString DefaultNickName;
    static QStringList DefaultChannels;
    static int DefaultMessageBufferLimit;

    static QString readIrcHostName(Host* pH);
    static int readIrcHostPort(Host* pH);
    static QString readIrcNickName(Host* pH);
    static QStringList readIrcChannels(Host* pH);
    static QPair<bool, QString> writeIrcHostName(Host* pH, const QString& hostname);
    static QPair<bool, QString> writeIrcHostPort(Host* pH, int port);
    static QPair<bool, QString> writeIrcNickName(Host* pH, const QString& nickname);
    static QPair<bool, QString> writeIrcChannels(Host* pH, const QStringList& channels);

    IrcConnection* connection;
    bool mReadyForSending;
    QPair<bool, QString> sendMsg(const QString& target, const QString& message);
    QString getHostName() { return mHostName; }
    int getHostPort() { return mHostPort; }
    QString getNickName() { return mNickName; }
    QStringList getChannels() { return mChannels; }
    QString getConnectedHost() { return mConnectedHostName; }
    void ircRestart(bool reloadConfigs = true);
    void setDefaultHostClient(bool isDefaultClient) { mIsDefaultIrcClient = isDefaultClient; }
    bool isDefaultHostClient() { return mIsDefaultIrcClient; }

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

    Host* mpHost;
    bool mIrcStarted;
    bool mIsDefaultIrcClient;
    IrcCompleter* completer;
    IrcCommandParser* commandParser;
    IrcBufferModel* bufferModel;
    QHash<IrcBuffer*, IrcUserModel*> userModels;
    QHash<IrcBuffer*, QTextDocument*> bufferTexts;
    QPointer<IrcBuffer> serverBuffer;
    QStringList mInputHistory;
    int mInputHistoryMax;
    int mInputHistoryIdxNext;
    int mInputHistoryIdxCurrent;
    quint64 mPingStarted;
    QString mConnectedHostName;
    QString mHostName;
    int mHostPort;
    QString mNickName;
    QString mUserName;
    QString mRealName;
    QStringList mChannels;
    int mMessageBufferLimit;
};

#endif // MUDLET_DLGIRC_H
