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
#include "post_guard.h"

class dlgIRC : public QMainWindow, public Ui::irc_dlg
{
    Q_OBJECT

    Q_DISABLE_COPY(dlgIRC)

public:
    dlgIRC();
    ~dlgIRC();

    IrcConnection* connection;
    bool sendMsg(const QString& target, const QString& message);

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
    void slot_receiveMessage(IrcMessage* message);
    void slot_onAnchorClicked(const QUrl& link);

private:
    void setupCommandParser();
    void setupBuffers();
    void processIrcMessage(IrcMessage*);
    bool processCustomCommand(IrcCommand*);
    void displayHelp(const QString&);

    IrcCompleter* completer;
    IrcCommandParser* commandParser;
    IrcBufferModel* bufferModel;
    QHash<IrcBuffer*, IrcUserModel*> userModels;
    QHash<IrcBuffer*, QTextDocument*> bufferTexts;
    quint64 mPingStarted;
    QString mHostName;
    int mHostPort;
    QString mNickName;
    QString mUserName;
    QString mRealName;
    QString mChannel;
};

#endif // MUDLET_DLGIRC_H
