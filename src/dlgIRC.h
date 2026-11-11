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


#include "ui_irc.h"
#include <Irc>
#include <IrcBuffer>
#include <IrcBufferModel>
#include <IrcCommand>
#include <IrcCommandParser>
#include <IrcCompleter>
#include <IrcMessage>
#include <IrcUserModel>

#include <QPointer>

#include "utils.h"

class Host;
class TIrcClient;

class dlgIRC : public QMainWindow, public Ui::irc
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgIRC)
    explicit dlgIRC(Host*);
    ~dlgIRC();

    inline static int DefaultMessageBufferLimit = 5000;

    QPair<bool, QString> sendMsg(const QString& target, const QString& message);

private slots:
    void slot_clientDestroyed();
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
    void slot_nickNameReserved(const QString& reserved, const QString& replacement);
    void slot_showMessage(IrcBuffer* buffer, IrcMessage* message);
    void slot_showOwnMessage(IrcMessage* message);
    void slot_onAnchorClicked(const QUrl& link);
    void slot_onHistoryCompletion();
    void slot_restarting(const QString& reason);
    void slot_restarted();

private:
    void setClientWindowTitle();
    void startClient();
    void setupCommandParser();
    void setupBuffers();
    bool processCustomCommand(IrcCommand*);
    void displayHelp(const QString&);
    void appendToDocument(QTextDocument*, const QString&);
    void writeQSettings();

    void showEvent(QShowEvent* event) override;

    QPointer<Host> mpHost;
    QPointer<TIrcClient> mpClient;
    bool mIrcStarted = false;
    IrcCompleter* completer = nullptr;
    IrcCommandParser* commandParser = nullptr;
    QHash<IrcBuffer*, IrcUserModel*> userModels;
    QHash<IrcBuffer*, QTextDocument*> bufferTexts;
    QStringList mInputHistory;
    int mInputHistoryMax = 8;
    int mInputHistoryIdxNext = 0;
    int mInputHistoryIdxCurrent = 0;
    int mMessageBufferLimit = 0;
};

#endif // MUDLET_DLGIRC_H
