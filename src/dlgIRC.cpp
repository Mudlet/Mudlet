/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2017-2018 by Stephen Lyons - slysven@virginmedia.com    *
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
#include <QDesktopServices>
#include <QScrollBar>
#include <QShortcut>
#include "post_guard.h"


QString dlgIRC::HostNameCfgItem = QStringLiteral("irc_host");
QString dlgIRC::HostPortCfgItem = QStringLiteral("irc_port");
QString dlgIRC::NickNameCfgItem = QStringLiteral("irc_nick");
QString dlgIRC::ChannelsCfgItem = QStringLiteral("irc_channels");
QString dlgIRC::DefaultHostName = QStringLiteral("irc.freenode.net");
int dlgIRC::DefaultHostPort = 6667;
QString dlgIRC::DefaultNickName = QStringLiteral("Mudlet");
QStringList dlgIRC::DefaultChannels = QStringList() << QStringLiteral("#mudlet");
int dlgIRC::DefaultMessageBufferLimit = 5000;

dlgIRC::dlgIRC(Host* pHost) : mReadyForSending(false), mpHost(pHost), mIrcStarted(false), mInputHistoryMax(8), mConnectedHostName()
{
    mInputHistoryMax = 8;
    mInputHistoryIdxNext = 0;
    mInputHistoryIdxCurrent = 0;

    setupUi(this);
    setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_irc.png")));

    bool isIntOk = false;
    mMessageBufferLimit = mudlet::self()->mpSettings->value("ircMessageBufferLimit", dlgIRC::DefaultMessageBufferLimit).toInt(&isIntOk);
    if (!isIntOk) {
        mMessageBufferLimit = dlgIRC::DefaultMessageBufferLimit;
    }

    setupCommandParser();

    connection = new IrcConnection(this);
    connection->setReconnectDelay(5);

    ircBrowser->setFocusProxy(lineEdit);

    // nick name completion & command history
    completer = new IrcCompleter(this);
    completer->setParser(commandParser);
    connect(completer, &IrcCompleter::completed, this, &dlgIRC::slot_nameCompleted);
    QShortcut* shortcut = new QShortcut(Qt::Key_Tab, this);
    QShortcut* shortcut2 = new QShortcut(Qt::Key_Up, this);
    connect(shortcut, &QShortcut::activated, this, &dlgIRC::slot_nameCompletion);
    connect(shortcut2, &QShortcut::activated, this, &dlgIRC::slot_onHistoryCompletion);
    connect(lineEdit, &QLineEdit::returnPressed, this, &dlgIRC::slot_onTextEntered);
    connect(lineEdit, &QLineEdit::textEdited, this, &dlgIRC::slot_onTextEdited);
    connect(ircBrowser, &QTextBrowser::anchorClicked, this, &dlgIRC::slot_onAnchorClicked);
    connect(userList, &QListView::doubleClicked, this, &dlgIRC::slot_onUserActivated);
    connect(connection, &IrcConnection::connected, this, &dlgIRC::slot_onConnected);
    connect(connection, &IrcConnection::connecting, this, &dlgIRC::slot_onConnecting);
    connect(connection, &IrcConnection::disconnected, this, &dlgIRC::slot_onDisconnected);
    connect(connection, &IrcConnection::nickNameRequired, this, &dlgIRC::slot_nickNameRequired);
    connect(connection, &IrcConnection::nickNameChanged, this, &dlgIRC::slot_nickNameChanged);
    connect(connection, &IrcConnection::joinMessageReceived, this, &dlgIRC::slot_joinedChannel);
    connect(connection, &IrcConnection::partMessageReceived, this, &dlgIRC::slot_partedChannel);
    connect(connection, &IrcConnection::numericMessageReceived, this, &dlgIRC::slot_receiveNumericMessage);

    mUserName = QStringLiteral("mudlet");
    mRealName = mudlet::self()->version;
    mHostName = readIrcHostName(mpHost);
    mHostPort = readIrcHostPort(mpHost);
    mNickName = readIrcNickName(mpHost);
    mChannels = readIrcChannels(mpHost);

    connection->setNickName(mNickName);
    connection->setUserName(mUserName);
    connection->setRealName(mRealName);
    connection->setHost(mHostName);
    connection->setPort(mHostPort);

    // set the title here to pick up the previously loaded nick and host values.
    setClientWindowTitle();
}

dlgIRC::~dlgIRC()
{
    writeQSettings();

    if (connection->isActive()) {
        const QString quitMsg = tr("%1 closed their client.").arg(mNickName);
        connection->quit(quitMsg);
        connection->close();
    }

    if (mudlet::self() && mudlet::self()->mpIrcClientMap.value(mpHost)) {
        mudlet::self()->mpIrcClientMap.remove(mpHost);
    }
}

void dlgIRC::setClientWindowTitle()
{
    setWindowTitle(tr("Mudlet IRC Client - %1 - %2 on %3").arg(mpHost->getName(), mNickName, mHostName));
}

void dlgIRC::startClient()
{
    if (mIrcStarted) {
        return;
    }

    connection->sendCommand(IrcCommand::createJoin(mChannels));
    connection->open();

    setupBuffers();

    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Starting Mudlet IRC Client...")));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Host: %1:%2").arg(mHostName, QString::number(mHostPort))));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Nick: %1").arg(mNickName)));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Auto-Join Channels: %1").arg(mChannels.join(" "))));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ This client supports Auto-Completion using the Tab key.")));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Type <b>/help</b> for commands or <b>/help [command]</b> for command syntax.")));
    ircBrowser->append(QStringLiteral("\n"));

    mIrcStarted = true;
}

QPair<bool, QString> dlgIRC::sendMsg(const QString& target, const QString& message)
{
    if (message.isEmpty()) {
        return QPair<bool, QString>(true, QStringLiteral("message processed by client"));
    }

    QString msgTarget = target;
    if (target.isEmpty()) {
        msgTarget = mChannels.first();
    }

    // inform the command parser of the target for this message.
    // parses the message and then reverts the target to avoid confusing our UI.
    QString lastParserTarget = commandParser->target();
    commandParser->setTarget(msgTarget);
    IrcCommand* command = commandParser->parse(message);
    commandParser->setTarget(lastParserTarget);

    if (!command) {
        return QPair<bool, QString>(false, QStringLiteral("message could not be parsed"));
    }

    bool isCustomCommand = processCustomCommand(command);
    if (isCustomCommand) {
        return QPair<bool, QString>(true, QStringLiteral("command processed by client"));
    }

    // update ping-started time if this command was a ping
    if (command->type() == IrcCommand::Ping) {
        mPingStarted = QDateTime::currentMSecsSinceEpoch();
    }

    connection->sendCommand(command);

    // if the command was a quit command we should close the IRC window.
    if (command->type() == IrcCommand::Quit) {
        setAttribute(Qt::WA_DeleteOnClose);
        close();
        return QPair<bool, QString>(true, QStringLiteral("closing client"));
    }

    // echo own messages (servers do not send our own messages back)
    if (command->type() == IrcCommand::Message || command->type() == IrcCommand::CtcpAction) {
        IrcMessage* msg = command->toMessage(connection->nickName(), connection);
        slot_receiveMessage(msg);
        delete msg;
    }

    return QPair<bool, QString>(true, QStringLiteral("sent to server"));
}

void dlgIRC::ircRestart(bool reloadConfigs)
{
    QString msg = tr("Restarting IRC Client");
    ircBrowser->append(IrcMessageFormatter::formatMessage("! %1.").arg(msg));

    // issue a quit message to the network if we're connected.
    if (connection->isConnected()) {
        connection->quit(msg);
    }

    // remove the old buffers.
    for (QString chName : qAsConst(mChannels)) {
        if (chName == serverBuffer->name()) {
            continue; // skip the server-buffer.
        }
        bufferModel->remove(chName);
    }

    connection->close();

    if (reloadConfigs) {
        mHostName = readIrcHostName(mpHost);
        mHostPort = readIrcHostPort(mpHost);
        mNickName = readIrcNickName(mpHost);
        mChannels = readIrcChannels(mpHost);

        connection->setNickName(mNickName);
        connection->setHost(mHostName);
        connection->setPort(mHostPort);
    }

    // queue auto-joined channels and reopen the connection.
    connection->sendCommand(IrcCommand::createJoin(mChannels));
    connection->open();

    serverBuffer->setName(connection->host());
    setClientWindowTitle();
}

void dlgIRC::setupCommandParser()
{
    // create a command parser and teach it some commands. notice also
    // that we must keep the command parser aware of the context in
    // setupBuffers() and onBufferActivated()
    commandParser = new IrcCommandParser(this);
    commandParser->setTolerant(true);
    commandParser->setTriggers(QStringList(QStringLiteral("/")));

    commandParser->addCommand(IrcCommand::CtcpAction, QStringLiteral("ACTION <target> <message...>"));
    commandParser->addCommand(IrcCommand::Admin, QStringLiteral("ADMIN (<server>)"));
    commandParser->addCommand(IrcCommand::Away, QStringLiteral("AWAY (<reason...>)"));
    commandParser->addCommand(IrcCommand::Info, QStringLiteral("INFO (<server>)"));
    commandParser->addCommand(IrcCommand::Invite, QStringLiteral("INVITE <user> (<#channel>)"));
    commandParser->addCommand(IrcCommand::Join, QStringLiteral("JOIN <#channel> (<key>)"));
    commandParser->addCommand(IrcCommand::Kick, QStringLiteral("KICK (<#channel>) <user> (<reason...>)"));
    commandParser->addCommand(IrcCommand::Knock, QStringLiteral("KNOCK <#channel> (<message...>)"));
    commandParser->addCommand(IrcCommand::List, QStringLiteral("LIST (<channels>) (<server>)"));
    commandParser->addCommand(IrcCommand::CtcpAction, QStringLiteral("ME [target] <message...>"));
    commandParser->addCommand(IrcCommand::Mode, QStringLiteral("MODE (<channel/user>) (<mode>) (<arg>)"));
    commandParser->addCommand(IrcCommand::Motd, QStringLiteral("MOTD (<server>)"));
    commandParser->addCommand(IrcCommand::Names, QStringLiteral("NAMES (<#channel>)"));
    commandParser->addCommand(IrcCommand::Nick, QStringLiteral("NICK <nick>"));
    commandParser->addCommand(IrcCommand::Notice, QStringLiteral("NOTICE <#channel/user> <message...>"));
    commandParser->addCommand(IrcCommand::Part, QStringLiteral("PART (<#channel>) (<message...>)"));
    commandParser->addCommand(IrcCommand::Ping, QStringLiteral("PING (<user>)"));
    commandParser->addCommand(IrcCommand::Quit, QStringLiteral("QUIT (<message...>)"));
    commandParser->addCommand(IrcCommand::Quote, QStringLiteral("QUOTE <command> (<parameters...>)"));
    commandParser->addCommand(IrcCommand::Stats, QStringLiteral("STATS <query> (<server>)"));
    commandParser->addCommand(IrcCommand::Time, QStringLiteral("TIME (<user>)"));
    commandParser->addCommand(IrcCommand::Topic, QStringLiteral("TOPIC (<#channel>) (<topic...>)"));
    commandParser->addCommand(IrcCommand::Trace, QStringLiteral("TRACE (<target>)"));
    commandParser->addCommand(IrcCommand::Users, QStringLiteral("USERS (<server>)"));
    commandParser->addCommand(IrcCommand::Version, QStringLiteral("VERSION (<user>)"));
    commandParser->addCommand(IrcCommand::Who, QStringLiteral("WHO <mask>"));
    commandParser->addCommand(IrcCommand::Whois, QStringLiteral("WHOIS <user>"));
    commandParser->addCommand(IrcCommand::Whowas, QStringLiteral("WHOWAS <user>"));

    commandParser->addCommand(IrcCommand::Custom, QStringLiteral("MSG <target> <message...>")); // replaces the old /msg command.
    commandParser->addCommand(IrcCommand::Custom, QStringLiteral("CLEAR (<buffer>)"));          // clears the given buffer, or the current active if none are given.
    commandParser->addCommand(IrcCommand::Custom, QStringLiteral("CLOSE (<buffer>)"));          // closes the buffer and removes it from the list, uses current active buffer if none are given.
    commandParser->addCommand(IrcCommand::Custom, QStringLiteral("RECONNECT"));                 // Issues a Quit command and closes the IRC connection then reconnects to the IRC server.
    commandParser->addCommand(IrcCommand::Custom, QStringLiteral("HELP (<command>)"));          // displays some help information about a given command or lists all available commands.
    commandParser->addCommand(IrcCommand::Custom, QStringLiteral("MSGLIMIT <limit> (<buffer>)"));  // sets buffer limit on all buffers and updates settings, or sets buffer limit on given buffer.
}

void dlgIRC::setupBuffers()
{
    bufferModel = new IrcBufferModel(connection);
    connect(bufferModel, &IrcBufferModel::added, this, &dlgIRC::slot_onBufferAdded);
    connect(bufferModel, &IrcBufferModel::removed, this, &dlgIRC::slot_onBufferRemoved);
    bufferList->setModel(bufferModel);
    // keep the command parser aware of the context
    connect(bufferModel, &IrcBufferModel::channelsChanged, commandParser, &IrcCommandParser::setChannels);
    // keep track of the current buffer, see also onBufferActivated()
    connect(bufferList->selectionModel(), &QItemSelectionModel::currentChanged, this, &dlgIRC::slot_onBufferActivated);
    // create a server buffer for non-targeted messages...
    serverBuffer = bufferModel->add(connection->host());
    serverBuffer->setName(connection->host());
    connect(bufferModel, &IrcBufferModel::messageIgnored, serverBuffer, &IrcBuffer::receiveMessage);
}

bool dlgIRC::processCustomCommand(IrcCommand* cmd)
{
    if (cmd->type() != IrcCommand::Custom || cmd->parameters().count() < 1) {
        return false;
    }

    const QString cmdName = QString(cmd->parameters().at(0)).toUpper();
    if (cmdName == "CLEAR") {
        auto * buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
        if (cmd->parameters().count() > 1) {
            QString bufferName = cmd->parameters().at(1);
            //QString cBufferName = buffer->title();
            if (!bufferName.isEmpty()) {
                buffer = bufferModel->find(bufferName);
            }
        }
        if (buffer) {
            bufferTexts.value(buffer)->clear();
        }
        return true;
    }
    if (cmdName == "CLOSE") {
        auto * buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
        if (cmd->parameters().count() > 1) {
            const QString bufferName = cmd->parameters().at(1);
            if (!bufferName.isEmpty()) {
                buffer = bufferModel->find(bufferName);
            }
        }
        if (buffer && buffer->title() != serverBuffer->title()) {
            bufferList->setCurrentIndex(bufferModel->index(bufferModel->find(connection->host())));
            buffer->close();
        }
        return true;
    }
    if (cmdName == "HELP") {
        QString hName = QString();
        if (cmd->parameters().count() > 1) {
            hName = QString(cmd->parameters().at(1)).toUpper();
        }
        displayHelp(hName);
        return true;
    }
    if (cmdName == "RECONNECT") {
        ircRestart();

        return true;
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
        return true;
    }
    if (cmdName == "MSGLIMIT") {
        int limit = 0;
        if (cmd->parameters().count() > 1) {
            bool isIntOk = false;
            limit = cmd->parameters().at(1).toInt(&isIntOk);
            if (!isIntOk) {
                limit = 0;
            }
        }
        if (limit <= 0) {
            QString error = tr("[Error] MSGLIMIT requires <limit> to be a whole number greater than zero!");
            ircBrowser->append(IrcMessageFormatter::formatMessage(error, QStringLiteral("indianred")));
            return true;
        }
        if (cmd->parameters().count() > 2) {
            QString bufferName = cmd->parameters().at(2);
            if (!bufferName.isEmpty()) {
                IrcBuffer* buffer = bufferModel->find(bufferName);
                if (buffer) {
                    auto* document = bufferTexts.value(buffer);
                    document->setMaximumBlockCount(limit);
                    return true;
                }
            }
        } else {
            for (auto* document : bufferTexts.values()) {
                document->setMaximumBlockCount(limit);
            }
            mMessageBufferLimit = limit;
            writeQSettings();
        }
    }

    return true;
}

void dlgIRC::displayHelp(const QString& cmdName = "")
{
    QString help;
    if (cmdName.isEmpty()) {
        help = tr("[HELP] Available Commands: %1").arg(commandParser->commands().join(QStringLiteral("  ")));
    } else {
        help = tr("[HELP] Syntax: %1").arg(commandParser->syntax(cmdName).replace(QStringLiteral("<"), QStringLiteral("&lt;")).replace(QStringLiteral(">"), QStringLiteral("&gt;")));
    }

    ircBrowser->append(IrcMessageFormatter::formatMessage(help));
}

void dlgIRC::slot_onConnected()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Connected to %1.")).arg(mHostName));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Joining %1...")).arg(mChannels.join(QStringLiteral(" "))));
}

void dlgIRC::slot_onConnecting()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Connecting %1...")).arg(mHostName));
}

void dlgIRC::slot_onDisconnected()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Disconnected from %1.")).arg(mHostName));
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
    }

    IrcCommand* command = commandParser->parse(input);
    if (command) {
        // handle custom commands
        bool isCustomCommand = processCustomCommand(command);
        if (isCustomCommand) {
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
            error = tr("[ERROR] Syntax: %1").arg(commandParser->syntax(command).replace(QStringLiteral("<"), QStringLiteral("&lt;")).replace(QStringLiteral(">"), QStringLiteral("&gt;")));
        else
            error = tr("[ERROR] Unknown command: %1").arg(command);
        ircBrowser->append(IrcMessageFormatter::formatMessage(error, QStringLiteral("indianred")));
        lineEdit->setStyleSheet(QStringLiteral("background: salmon"));
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
        mInputHistoryIdxCurrent = 0;
    }

    if (mInputHistory.count() == 0) {
        return;
    }

    lineEdit->setText(mInputHistory.at(mInputHistoryIdxCurrent));
    ++mInputHistoryIdxCurrent;
}

void dlgIRC::slot_onBufferAdded(IrcBuffer* buffer)
{
    // joined a buffer - start listening to buffer specific messages
    connect(buffer, &IrcBuffer::messageReceived, this, &dlgIRC::slot_receiveMessage);
    // create a document for storing the buffer specific messages
    auto * document = new QTextDocument(buffer);
    document->setMaximumBlockCount(mMessageBufferLimit);
    bufferTexts.insert(buffer, document);
    // create a sorted model for buffer users
    auto * userModel = new IrcUserModel(buffer);
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
    auto * buffer = index.data(Irc::BufferRole).value<IrcBuffer*>();
    // document, user list and nick completion for the current buffer
    ircBrowser->setDocument(bufferTexts.value(buffer));
    ircBrowser->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    userList->setModel(userModels.value(buffer));
    completer->setBuffer(buffer);
    // keep the command parser aware of the context
    if (buffer) {
        commandParser->setTarget(buffer->title());
    }
}

void dlgIRC::slot_onUserActivated(const QModelIndex& index)
{
    auto * user = index.data(Irc::UserRole).value<IrcUser*>();
    if (user) {
        // ensure the "user" isn't our own client, can only do this by name.
        if (user->name() == mNickName) {
            return;
        }
        IrcBuffer* buffer = bufferModel->add(user->name());
        // activate the new query
        int idx = bufferModel->buffers().indexOf(buffer);
        if (idx != -1) {
            bufferList->setCurrentIndex(bufferModel->index(idx));
        }
    }
}

void dlgIRC::appendHtml(QTextDocument* document, const QString& html)
{
    QTextCursor cursor(document);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);
    if (!document->isEmpty()) {
        cursor.insertBlock();
    }
    cursor.insertHtml(html);
    cursor.endEditBlock();
}

void dlgIRC::slot_receiveMessage(IrcMessage* message)
{
    // update timestamp of ping/pong messages.
    if (message->type() == IrcMessage::Pong && mPingStarted) {
        message->setTimeStamp(QDateTime::fromMSecsSinceEpoch(mPingStarted));
        mPingStarted = 0;
    }

    auto * buffer = qobject_cast<IrcBuffer*>(sender());
    if (!buffer) {
        buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
    }
    QTextDocument* document = bufferTexts.value(buffer);
    if (document) {
        QString html = IrcMessageFormatter::formatMessage(message);
        if (!html.isEmpty()) {
            // send a plain-text formatted copy of the message to Lua, as long as it isn't our own.
            if (!message->isOwn()) {
                QString textToLua = IrcMessageFormatter::formatMessage(message, true);
                if (!textToLua.isEmpty()) {
                    QString from = message->nick();
                    QString to = getMessageTarget(message, buffer->title());
                    if (!isDefaultHostClient()) {
                        mpHost->postIrcMessage(from, to, textToLua);
                    }
                }
            }

            // add the HTML formatted copy to the buffer.
            if (document == ircBrowser->document()) {
                ircBrowser->append(html);
            } else {
                dlgIRC::appendHtml(document, html);
            }
        }
    }
}

void dlgIRC::slot_onAnchorClicked(const QUrl& link)
{
    QDesktopServices::openUrl(link);
}

void dlgIRC::slot_nickNameRequired(const QString& reserved, QString* alt)
{
    QString newNick = QStringLiteral("%1_%2").arg(reserved, QString::number(rand() % 10000));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! The Nickname %1 is reserved. Automatically changing Nickname to: %2").arg(reserved, newNick)));
    connection->setNickName(newNick);
}

void dlgIRC::slot_nickNameChanged(const QString& nick)
{
    if (nick == mNickName) {
        return;
    }

    // send a notice to Lua about the nick name change.
    if (!isDefaultHostClient()) {
        mpHost->postIrcMessage(mNickName, nick, tr("Your nick has changed."));
    }
    mNickName = nick;

    setClientWindowTitle();
}

void dlgIRC::slot_joinedChannel(IrcJoinMessage* message)
{
    if (!mReadyForSending) {
        mReadyForSending = true;
    }

    QString chan = message->channel();
    if (!mChannels.contains(chan)) {
        mChannels << chan;
    }

    if (message->isOwn()) {
        QString luaText = IrcMessageFormatter::formatMessage(static_cast<IrcMessage*>(message), true);
        if (!isDefaultHostClient()) {
            mpHost->postIrcMessage(message->nick(), message->channel(), luaText);
        }
    }
}

void dlgIRC::slot_partedChannel(IrcPartMessage* message)
{
    QString chan = message->channel();
    if (mChannels.contains(chan)) {
        mChannels.removeAll(chan);
    }

    if (message->isOwn() && !isDefaultHostClient()) {
        QString luaText = IrcMessageFormatter::formatMessage(static_cast<IrcMessage*>(message), true);
        mpHost->postIrcMessage(message->nick(), message->channel(), luaText);
    }
}

void dlgIRC::slot_receiveNumericMessage(IrcNumericMessage* msg)
{
    // set the connected host name and update the serverBuffer name to match it.
    if (msg->code() == Irc::RPL_YOURHOST) {
        serverBuffer->setName(msg->nick());
        mConnectedHostName = msg->nick();
    }
}

void dlgIRC::showEvent(QShowEvent* event)
{
    startClient();
    event->ignore();
}

QString dlgIRC::getMessageTarget(IrcMessage* msg, const QString& bufferName)
{
    QString target = bufferName;
    switch (msg->type()) {
    case IrcMessage::Notice: {
        auto * msgNotice = static_cast<IrcNoticeMessage*>(msg);
        target = msgNotice->target();
        break;
    }
    case IrcMessage::Private: {
        auto * msgPrivate = static_cast<IrcPrivateMessage*>(msg);
        target = msgPrivate->target();
        break;
    }
    }
    return target;
}

QString dlgIRC::readIrcHostName(Host* pH)
{
    QString hostname = pH->readProfileData(dlgIRC::HostNameCfgItem);
    if (hostname.isEmpty()) {
        hostname = dlgIRC::DefaultHostName;
    }
    return hostname;
}

int dlgIRC::readIrcHostPort(Host* pH)
{
    QString portStr = pH->readProfileData(dlgIRC::HostPortCfgItem);
    bool ok;
    int port = portStr.toInt(&ok);
    if (portStr.isEmpty() || !ok) {
        port = dlgIRC::DefaultHostPort;
    } else if (port > 65535 || port < 1) {
        port = dlgIRC::DefaultHostPort;
    }
    return port;
}

QString dlgIRC::readIrcNickName(Host* pH)
{
    QString nick = pH->readProfileData(dlgIRC::NickNameCfgItem);
    if (nick.isEmpty()) {
        // if the new config doesn't exist, try loading the old one.
        nick = readAppDefaultIrcNick();

        if (nick.isEmpty()) {
            nick = QStringLiteral("%1%2").arg(dlgIRC::DefaultNickName, QString::number(rand() % 10000));
        }
    }
    return nick;
}

QString dlgIRC::readAppDefaultIrcNick()
{
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("irc_nick")));
    bool opened = file.open(QIODevice::ReadOnly);
    QString rstr;
    if (opened) {
        QDataStream ifs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ifs >> rstr;
        file.close();
    }
    return rstr;
}

void dlgIRC::writeAppDefaultIrcNick(const QString& nick)
{
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("irc_nick")));
    bool opened = file.open(QIODevice::WriteOnly);
    if (opened) {
        QDataStream ofs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ofs << nick;
        file.close();
    }
}

QStringList dlgIRC::readIrcChannels(Host* pH)
{
    QStringList channels;
    QString channelstr = pH->readProfileData(dlgIRC::ChannelsCfgItem);
    if (channelstr.isEmpty()) {
        channels << dlgIRC::DefaultChannels;
    } else {
        channels = channelstr.split(QStringLiteral(" "), QString::SkipEmptyParts);
    }
    return channels;
}

QPair<bool, QString> dlgIRC::writeIrcHostName(Host* pH, const QString& hostname)
{
    return pH->writeProfileData(dlgIRC::HostNameCfgItem, hostname);
}

QPair<bool, QString> dlgIRC::writeIrcHostPort(Host* pH, int port)
{
    return pH->writeProfileData(dlgIRC::HostPortCfgItem, QString::number(port));
}

QPair<bool, QString> dlgIRC::writeIrcNickName(Host* pH, const QString& nickname)
{
    // update app-wide file to set a default nick as whatever the last-used nick was.
    writeAppDefaultIrcNick(nickname);

    return pH->writeProfileData(dlgIRC::NickNameCfgItem, nickname);
}

QPair<bool, QString> dlgIRC::writeIrcChannels(Host* pH, const QStringList& channels)
{
    return pH->writeProfileData(dlgIRC::ChannelsCfgItem, channels.join(QStringLiteral(" ")));
}

void dlgIRC::writeQSettings()
{
    if (mudlet::self()) {
        mudlet::self()->mpSettings->setValue("ircMessageBufferLimit", mMessageBufferLimit);
    }
}
