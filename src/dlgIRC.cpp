/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "dlgIRC.h"
#include "Host.h"
#include "ircmessageformatter.h"

#include "mudlet.h"

#include "pre_guard.h"
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QScrollBar>
#include <QShortcut>
#include <QString>
#include <QTextDocument>
#include <QTime>
#include "post_guard.h"


dlgIRC::dlgIRC(Host* pHost) :
  mpHost(pHost)
, mInputHistoryMax(8)
{
    setupUi(this);
    setWindowTitle(tr("%1 - Mudlet IRC Client").arg(mpHost->getName()));

    setupCommandParser();

    connection = new IrcConnection(this);
    connection->setReconnectDelay(5);

    lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    ircBrowser->setFocusProxy( lineEdit );

    // nick name completion
    completer = new IrcCompleter(this);
    completer->setParser(commandParser);
    connect(completer, SIGNAL(completed(QString,int)), this, SLOT(slot_nameCompleted(QString,int)));
    QShortcut* shortcut = new QShortcut(Qt::Key_Tab, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(slot_nameCompletion()));
    QShortcut* shortcut2 = new QShortcut(Qt::Key_Up, this);
    connect(shortcut2, SIGNAL(activated()), this, SLOT(slot_onHistoryCompletion()));

    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(slot_onTextEntered()));
    connect(lineEdit, SIGNAL(textEdited(QString)), this, SLOT(slot_onTextEdited()));
    connect(ircBrowser, SIGNAL(anchorClicked(QUrl)), this, SLOT(slot_onAnchorClicked(QUrl)));
    connect(userList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slot_onUserActivated(QModelIndex)));
    connect(connection, SIGNAL(connected()), this, SLOT(slot_onConnected()));
    connect(connection, SIGNAL(connecting()), this, SLOT(slot_onConnecting()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(slot_onDisconnected()));
    connect(connection, SIGNAL(nickNameRequired(QString,QString*)), this, SLOT(slot_nickNameRequired(QString,QString*)));
    connect(connection, SIGNAL(nickNameChanged(QString)), this, SLOT(slot_nickNameChanged(QString)));
    connect(connection, SIGNAL(joinMessageReceived(IrcJoinMessage*)), this, SLOT(slot_joinedChannel(IrcJoinMessage*)));
    connect(connection, SIGNAL(partMessageReceived(IrcPartMessage*)), this, SLOT(slot_partedChannel(IrcPartMessage*)));

    mNickName = loadNickName();
    mUserName = "mudlet";
    mRealName = mudlet::self()->version;
    mHostName = loadHostName();
    mHostPort = loadHostPort();
    mChannels = loadChannels();

    connection->setNickName(mNickName);
    connection->setUserName(mUserName);
    connection->setRealName(mRealName);
    connection->setHost(mHostName);
    connection->setPort(mHostPort);

    connection->sendCommand(IrcCommand::createJoin(mChannels));
    connection->open();

    setupBuffers();

    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Statring Mudlet IRC Client...")));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Host: %1:%2").arg(mHostName, QString::number(mHostPort))));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Nick: %1").arg(mNickName)));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Auto-Join Channels: %1").arg(mChannels.join(" "))));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ This client supports Auto-Completion using the Tab key.")));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Type <b>/help</b> for commands or <b>/help [command]</b> for command syntax.")));
    ircBrowser->append("\n");
}

dlgIRC::~dlgIRC()
{
    if (connection->isActive()) {
        const QString quitMsg = tr("%1 closed their client.").arg(mNickName);
        connection->quit(quitMsg);
        connection->close();
    }
}

QString dlgIRC::loadHostName()
{
    QString hostname = mpHost->readProfileData("irc_hostname");
    if (hostname.isEmpty()) {
        hostname = "irc.freenode.net";
    }
    return hostname;
}

int dlgIRC::loadHostPort()
{
    QString portStr = mpHost->readProfileData("irc_hostport");
    bool ok;
    int port = portStr.toInt(&ok);
    if (portStr.isEmpty() || !ok) {
        port = 6667;
    } else if (port > 65535 || port < 1) {
        port = 6667;
    }
    return port;
}

QString dlgIRC::loadNickName()
{
    QString nick = mpHost->readProfileData("irc_nickname");
    if (nick.isEmpty()) {
        qsrand(QTime::currentTime().msec());
        nick = tr("Mudlet%1").arg(QString::number(rand() % 10000));
    }
    return nick;
}

QStringList dlgIRC::loadChannels()
{
    QStringList channels;
    QString channelstr = mpHost->readProfileData("irc_channels");
    if (channelstr.isEmpty()) {
        channels << "#mudlet";
    } else {
        channels = channelstr.split(" ", QString::SkipEmptyParts);
    }
    return channels;
}

QPair<bool, QString> dlgIRC::saveHostName(const QString& hostname)
{
    return mpHost->writeProfileData("irc_hostname", hostname);
}

QPair<bool, QString> dlgIRC::saveHostPort(int port)
{
    return mpHost->writeProfileData("irc_hostport", QString::number(port));
}

QPair<bool, QString> dlgIRC::saveNickName(const QString& nickname)
{
    return mpHost->writeProfileData("irc_nickname", nickname);
}

QPair<bool, QString> dlgIRC::saveChannels(const QStringList& channels)
{
    return mpHost->writeProfileData("irc_channels", channels.join(" "));
}

bool dlgIRC::sendMsg(const QString &target, const QString &message)
{
    // I think it would be interesting to see a command parser used here.
    // Maybe doing that would be an easier way to expose IRC commands to Lua
    // without the need to write more code in the lua interpreter.
    IrcCommand* command = IrcCommand::createMessage( target, message );
    bool rv = connection->sendCommand(command);
    // echo own messages (servers do not send our own messages back)
    if (command->type() == IrcCommand::Message || command->type() == IrcCommand::CtcpAction) {
        IrcMessage* msg = command->toMessage(connection->nickName(), connection);
        slot_receiveMessage(msg);
        delete msg;
    }
    return rv;
}

void dlgIRC::ircRestart(bool reloadConfigs)
{
    QString msg = tr("Restart IRC Client");
    ircBrowser->append(IrcMessageFormatter::formatMessage("! %1.").arg(msg));
    if (connection->isConnected())
        connection->quit(msg);
    connection->close();

    if (reloadConfigs) {
        mHostName = loadHostName();
        mHostPort = loadHostPort();
        mNickName = loadNickName();
        mChannels = loadChannels();

        connection->setNickName(mNickName);
        connection->setHost(mHostName);
        connection->setPort(mHostPort);
    }

    // queue auto-joined channels and reopen the connection.
    connection->sendCommand(IrcCommand::createJoin(mChannels));
    connection->open();
}

void dlgIRC::setupCommandParser() {
    // create a command parser and teach it some commands. notice also
    // that we must keep the command parser aware of the context in
    // setupBuffers() and onBufferActivated()
    commandParser = new IrcCommandParser(this);
    commandParser->setTolerant(true);
    commandParser->setTriggers(QStringList("/"));

    commandParser->addCommand(IrcCommand::CtcpAction, "ACTION <target> <message...>");
    commandParser->addCommand(IrcCommand::Admin, "ADMIN (<server>)");
    commandParser->addCommand(IrcCommand::Away, "AWAY (<reason...>)");
    commandParser->addCommand(IrcCommand::Info, "INFO (<server>)");
    commandParser->addCommand(IrcCommand::Invite, "INVITE <user> (<#channel>)");
    commandParser->addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
    commandParser->addCommand(IrcCommand::Kick, "KICK (<#channel>) <user> (<reason...>)");
    commandParser->addCommand(IrcCommand::Knock, "KNOCK <#channel> (<message...>)");
    commandParser->addCommand(IrcCommand::List, "LIST (<channels>) (<server>)");
    commandParser->addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
    commandParser->addCommand(IrcCommand::Mode, "MODE (<channel/user>) (<mode>) (<arg>)");
    commandParser->addCommand(IrcCommand::Motd, "MOTD (<server>)");
    commandParser->addCommand(IrcCommand::Names, "NAMES (<#channel>)");
    commandParser->addCommand(IrcCommand::Nick, "NICK <nick>");
    commandParser->addCommand(IrcCommand::Notice, "NOTICE <#channel/user> <message...>");
    commandParser->addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
    commandParser->addCommand(IrcCommand::Ping, "PING (<user>)");
    commandParser->addCommand(IrcCommand::Quit, "QUIT (<message...>)");
    commandParser->addCommand(IrcCommand::Quote, "QUOTE <command> (<parameters...>)");
    commandParser->addCommand(IrcCommand::Stats, "STATS <query> (<server>)");
    commandParser->addCommand(IrcCommand::Time, "TIME (<user>)");
    commandParser->addCommand(IrcCommand::Topic, "TOPIC (<#channel>) (<topic...>)");
    commandParser->addCommand(IrcCommand::Trace, "TRACE (<target>)");
    commandParser->addCommand(IrcCommand::Users, "USERS (<server>)");
    commandParser->addCommand(IrcCommand::Version, "VERSION (<user>)");
    commandParser->addCommand(IrcCommand::Who, "WHO <mask>");
    commandParser->addCommand(IrcCommand::Whois, "WHOIS <user>");
    commandParser->addCommand(IrcCommand::Whowas, "WHOWAS <user>");

    commandParser->addCommand(IrcCommand::Custom, "MSG <target> <message...>");  // replaces the old /msg command.
    commandParser->addCommand(IrcCommand::Custom, "CLEAR (<buffer>)");  // clears the given buffer, or the current active if none are given.
    commandParser->addCommand(IrcCommand::Custom, "CLOSE (<buffer>)");  // closes the buffer and removes it from the list, uses current active buffer if none are given.
    commandParser->addCommand(IrcCommand::Custom, "RECONNECT");  // Issues a Quit command and closes the IRC connection then reconnects to the IRC server.
    commandParser->addCommand(IrcCommand::Custom, "HELP (<command>)");  // displays some help information about a given command or lists all available commands.
}

void dlgIRC::setupBuffers()
{
    bufferModel = new IrcBufferModel(connection);
    connect(bufferModel, SIGNAL(added(IrcBuffer*)), this, SLOT(slot_onBufferAdded(IrcBuffer*)));
    connect(bufferModel, SIGNAL(removed(IrcBuffer*)), this, SLOT(slot_onBufferRemoved(IrcBuffer*)));
    bufferList->setModel(bufferModel);
    // keep the command parser aware of the context
    connect(bufferModel, SIGNAL(channelsChanged(QStringList)), commandParser, SLOT(setChannels(QStringList)));
    // keep track of the current buffer, see also onBufferActivated()
    connect(bufferList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slot_onBufferActivated(QModelIndex)));
    // create a server buffer for non-targeted messages...
    IrcBuffer* serverBuffer = bufferModel->add( connection->host() );
    connect(bufferModel, SIGNAL(messageIgnored(IrcMessage*)), serverBuffer, SLOT(receiveMessage(IrcMessage*)));
}

bool dlgIRC::processCustomCommand(IrcCommand* cmd)
{
    if (cmd->type() != IrcCommand::Custom || cmd->parameters().count() < 1) {
        return true;
    }

    const QString cmdName = QString(cmd->parameters().at(0)).toUpper();
    if (cmdName == "CLEAR") {
        IrcBuffer* buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
        if (cmd->parameters().count() > 1) {
            QString bufferName = cmd->parameters().at(1);
            //QString cBufferName = buffer->title();
            if (!bufferName.isEmpty()) {
                buffer = bufferModel->find(bufferName);
            }
        }
        if(buffer) {
            bufferTexts.value(buffer)->clear();
        }
        return false;
    }
    if (cmdName == "CLOSE") {
        IrcBuffer* buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
        if (cmd->parameters().count() > 1) {
            const QString bufferName = cmd->parameters().at(1);
            if (!bufferName.isEmpty()) {
                buffer = bufferModel->find(bufferName);
            }
        }
        if (buffer) {
            bufferList->setCurrentIndex(bufferModel->index(bufferModel->find(connection->host())));
            //bufferModel->remove(buffer);
            buffer->close();
        }
        return false;
    }
    if (cmdName == "HELP") {
        QString hName = "";
        if (cmd->parameters().count() > 1) {
            hName = QString(cmd->parameters().at(1)).toUpper();
        }
        displayHelp(hName);
        return false;
    }
    if (cmdName == "RECONNECT") {
        ircRestart();

        return false;
    }
    if (cmdName == "MSG") {
        QString target;
        QString msgText;
        if (cmd->parameters().count() > 1) {
            target = QString(cmd->parameters().at(1));
        }
        if (target.isEmpty()) {
            target = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>()->title();
        }
        if (cmd->parameters().count() > 2) {
            msgText = QString(cmd->parameters().mid(2).join(" "));
        }

        sendMsg(target, msgText);
    }

    return false;
}

void dlgIRC::displayHelp(const QString& cmdName = "") {
    QString help;
    if (cmdName.isEmpty()) {
        help = tr("[HELP] Available Commands: %1").arg( commandParser->commands().join("  ") );
    } else {
        help = tr("[HELP] Syntax: %1").arg(commandParser->syntax(cmdName).replace("<", "&lt;").replace(">", "&gt;"));
    }

    ircBrowser->append(IrcMessageFormatter::formatMessage( help ));
}

void dlgIRC::slot_onConnected()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage("! Connected to %1.").arg(mHostName));
    ircBrowser->append(IrcMessageFormatter::formatMessage("! Joining %1...").arg(mChannels.join(" ")));
}

void dlgIRC::slot_onConnecting()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage("! Connecting %1...").arg(mHostName));
}

void dlgIRC::slot_onDisconnected()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage("! Disconnected from %1.").arg(mHostName));
}

void dlgIRC::slot_onTextEdited()
{
    // clear the possible error indication
    lineEdit->setStyleSheet(QString());
}

void dlgIRC::slot_onTextEntered()
{
    QString input = lineEdit->text();

    // add this line to our history list.
    if (!input.isEmpty()) {
        if (mInputHistoryIdxNext >= mInputHistoryMax) {
            mInputHistoryIdxNext = 0;
        }
        if (mInputHistory.count() > mInputHistoryIdxNext) {
            mInputHistory[mInputHistoryIdxNext] = input;
        } else {
            mInputHistory << input;
        }
        mInputHistoryIdxCurrent = mInputHistoryIdxNext;
        ++mInputHistoryIdxNext;

        qDebug() << "History Count:"<< mInputHistory.count();
    }

    IrcCommand* command = commandParser->parse(input);
    if (command) {
        // handle custom commands
        bool sendCommand = processCustomCommand(command);
        if( !sendCommand ) {
            lineEdit->clear();
            return;
        }

        // update ping-started time if this command was a ping
        if (command->type() == IrcCommand::Ping) {
            mPingStarted = QDateTime::currentMSecsSinceEpoch();
        }

        // send to the server.
        connection->sendCommand(command);

        // if the command was a quit command we should close this window.
        if (command->type() == IrcCommand::Quit) {
            setAttribute(Qt::WA_DeleteOnClose);
            close();
            return;
        }

        // echo own messages (servers do not send our own messages back)
        if (command->type() == IrcCommand::Message || command->type() == IrcCommand::CtcpAction) {
            IrcMessage* msg = command->toMessage(connection->nickName(), connection);
            slot_receiveMessage(msg);
            delete msg;
        }
        lineEdit->clear();
    } else if (input.length() > 1) {
        QString error;
        QString command = lineEdit->text().mid(1).split(" ", QString::SkipEmptyParts).value(0).toUpper();
        if (commandParser->commands().contains(command))
            error = tr("[ERROR] Syntax: %1").arg(commandParser->syntax(command).replace("<", "&lt;").replace(">", "&gt;"));
        else
            error = tr("[ERROR] Unknown command: %1").arg(command);
        ircBrowser->append(IrcMessageFormatter::formatMessage(error, "indianred"));
        lineEdit->setStyleSheet("background: salmon");
    }
}

void dlgIRC::slot_nameCompletion()
{
    completer->complete(lineEdit->text(), lineEdit->cursorPosition());
}

void dlgIRC::slot_nameCompleted(const QString& text, int cursor)
{
    lineEdit->setText(text);
    lineEdit->setCursorPosition(cursor);
}

void dlgIRC::slot_onHistoryCompletion()
{
    if (mInputHistoryIdxCurrent >= mInputHistory.count()) {
        qDebug() << "reset current index" << mInputHistoryIdxCurrent;
        mInputHistoryIdxCurrent = 0;
    }

    if (mInputHistory.count() == 0) {
        return;
    }

    lineEdit->setText( mInputHistory.at(mInputHistoryIdxCurrent) );
    ++mInputHistoryIdxCurrent;
}

void dlgIRC::slot_onBufferAdded(IrcBuffer* buffer)
{
    // joined a buffer - start listening to buffer specific messages
    connect(buffer, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(slot_receiveMessage(IrcMessage*)));
    // create a document for storing the buffer specific messages
    QTextDocument* document = new QTextDocument(buffer);
    bufferTexts.insert(buffer, document);
    // create a sorted model for buffer users
    IrcUserModel* userModel = new IrcUserModel(buffer);
    userModel->setSortMethod(Irc::SortByTitle);
    userModels.insert(buffer, userModel);
    // activate the new buffer
    int idx = bufferModel->buffers().indexOf(buffer);
    if (idx != -1)
        bufferList->setCurrentIndex(bufferModel->index(idx));
}

void dlgIRC::slot_onBufferRemoved(IrcBuffer* buffer)
{
    // the buffer specific models and documents are no longer needed
    delete userModels.take(buffer);
    delete bufferTexts.take(buffer);
}

void dlgIRC::slot_onBufferActivated(const QModelIndex& index)
{
    IrcBuffer* buffer = index.data(Irc::BufferRole).value<IrcBuffer*>();
    // document, user list and nick completion for the current buffer
    ircBrowser->setDocument(bufferTexts.value(buffer));
    ircBrowser->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    userList->setModel(userModels.value(buffer));
    completer->setBuffer(buffer);
    // keep the command parser aware of the context
    if (buffer)
        commandParser->setTarget(buffer->title());
}

void dlgIRC::slot_onUserActivated(const QModelIndex& index)
{
    IrcUser* user = index.data(Irc::UserRole).value<IrcUser*>();
    if (user) {
        // ensure the "user" isn't our own client, can only do this by name.
        if( user->name() == mNickName ) {
            return;
        }
        IrcBuffer* buffer = bufferModel->add(user->name());
        // activate the new query
        int idx = bufferModel->buffers().indexOf(buffer);
        if (idx != -1)
            bufferList->setCurrentIndex(bufferModel->index(idx));
    }
}

static void appendHtml(QTextDocument* document, const QString& html)
{
    QTextCursor cursor(document);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);
    if (!document->isEmpty())
        cursor.insertBlock();
    cursor.insertHtml(html);
    cursor.endEditBlock();
}

void dlgIRC::slot_receiveMessage(IrcMessage* message)
{
    // update timestamp of ping/pong messages.
    if (message->type() == IrcMessage::Pong && mPingStarted) {
        message->setTimeStamp( QDateTime::fromMSecsSinceEpoch(mPingStarted) );
        mPingStarted = 0;
    }

    // Handle posting IRC related events for messages sent to us.
    if( message->flags() ^ IrcMessage::Own ) {
        processIrcMessage( message );
    }

    IrcBuffer* buffer = qobject_cast<IrcBuffer*>(sender());
    if (!buffer)
        buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
    QTextDocument* document = bufferTexts.value(buffer);
    if (document) {
        QString html = IrcMessageFormatter::formatMessage(message);
        if (!html.isEmpty()) {
            if (document == ircBrowser->document())
                ircBrowser->append(html);
            else
                appendHtml(document, html);
        }
    }
}

void dlgIRC::processIrcMessage(IrcMessage* msg)
{
    // TODO FIXME we should replace this with a variation to the messageFormatter
    //  which simply provides a plain-text variation of the IRC texts for posting to lua.
    IrcMessage::Type msgType = msg->type();
    switch( msgType ) {
    case IrcMessage::Notice: {
        IrcNoticeMessage* msgNotice = static_cast<IrcNoticeMessage*>(msg);
        const QString content = IrcTextFormat().toPlainText(msgNotice->content());
        mpHost->postIrcMessage(msgNotice->nick(), msgNotice->target(), content);
        break;
    }
    case IrcMessage::Private : {
        IrcPrivateMessage* msgPrivate = static_cast<IrcPrivateMessage*>(msg);
        const QString content = IrcTextFormat().toPlainText(msgPrivate->content());
        mpHost->postIrcMessage(msgPrivate->nick(), msgPrivate->target(), content);
        break;
    }
    }
}

void dlgIRC::slot_onAnchorClicked(const QUrl& link)
{
    QDesktopServices::openUrl(link);
}

void dlgIRC::slot_nickNameRequired(const QString &reserved, QString *alt)
{
    QString newNick = QString("%1_%2").arg(reserved, QString::number(rand() % 10000) );
    ircBrowser->append(IrcMessageFormatter::formatMessage("! The Nickname %1 is reserved. Automatically changing Nickname to: %2").arg(reserved, newNick));
    connection->setNickName( newNick );
}

void dlgIRC::slot_nickNameChanged(const QString& nick)
{
    if( nick == mNickName ) {
        return;
    }

    // send a notice to Lua about the nick name change.
    mpHost->postIrcMessage(mNickName, nick, tr("Your nick has changed."));
    mNickName = nick;
}

void dlgIRC::slot_joinedChannel(IrcJoinMessage* message)
{
    QString chan = message->channel();
    if (!mChannels.contains(chan)) {
        mChannels << chan;
    }
}

void dlgIRC::slot_partedChannel(IrcPartMessage *message)
{
    QString chan = message->channel();
    if (mChannels.contains(chan)) {
        mChannels.removeAll(chan);
    }
}
