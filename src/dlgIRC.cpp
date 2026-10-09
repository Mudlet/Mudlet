/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2017-2018, 2020, 2022, 2024 by Stephen Lyons            *
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


#include "dlgIRC.h"
#include "Host.h"
#include "MudletApp.h"
#include "TIrcClient.h"
#include "ircmessageformatter.h"

#include <IrcConnection>
#include <IrcTextFormat>
#include <IrcUser>

#include "mudlet.h"

#include <QDesktopServices>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>


dlgIRC::dlgIRC(Host* pHost)
: mpHost(pHost)
, mpClient(pHost->getOrCreateIrcClient())
{
    setupUi(this);
    setWindowIcon(QIcon(qsl(":/icons/mudlet_irc.png")));

    bool isIntOk = false;
    mMessageBufferLimit = MudletApp::getQSettings()->value("ircMessageBufferLimit", dlgIRC::DefaultMessageBufferLimit).toInt(&isIntOk);
    if (!isIntOk) {
        mMessageBufferLimit = dlgIRC::DefaultMessageBufferLimit;
    }

    setupCommandParser();

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
    IrcConnection* connection = mpClient->connection();
    connect(connection, &IrcConnection::connected, this, &dlgIRC::slot_onConnected);
    connect(connection, &IrcConnection::connecting, this, &dlgIRC::slot_onConnecting);
    connect(connection, &IrcConnection::disconnected, this, &dlgIRC::slot_onDisconnected);
    connect(mpClient, &TIrcClient::signal_nickNameReserved, this, &dlgIRC::slot_nickNameReserved);
    connect(mpClient, &TIrcClient::signal_nickNameChanged, this, &dlgIRC::setClientWindowTitle);
    connect(mpClient, &TIrcClient::signal_restarting, this, &dlgIRC::slot_restarting);
    connect(mpClient, &TIrcClient::signal_restarted, this, &dlgIRC::slot_restarted);
    connect(mpClient, &TIrcClient::signal_messageSent, this, &dlgIRC::slot_showOwnMessage);
    connect(mpClient, &TIrcClient::signal_messageReceived, this, &dlgIRC::slot_showMessage);
    connect(mpClient, &QObject::destroyed, this, &dlgIRC::slot_clientDestroyed);

    // set the title here to pick up the previously loaded nick and host values.
    setClientWindowTitle();
}

dlgIRC::~dlgIRC()
{
    writeQSettings();

    // The session ends with its window, which must not hear it disconnect or go
    if (mpClient) {
        ircBrowser->setDocument(nullptr);
        mpClient->connection()->disconnect(this);
        mpClient->disconnect(this);
        delete mpClient.data();
    }

    if (mpHost && mpHost->mpDlgIRC) {
        mpHost->mpDlgIRC = nullptr;
    }
}

// The session's buffers own the documents shown here and go with it, but the
// browser does not notice its document being deleted.
void dlgIRC::slot_clientDestroyed()
{
    ircBrowser->setDocument(nullptr);
    hide();
}

void dlgIRC::setClientWindowTitle()
{
    setWindowTitle(tr("Mudlet IRC Client - %1 - %2 on %3").arg(mpHost->getName(), mpClient->getNickName(), mpClient->getHostName()));
}

void dlgIRC::startClient()
{
    if (mIrcStarted) {
        return;
    }

    mpClient->start();

    setupBuffers();

    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Starting Mudlet IRC Client...")));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Host: %1:%2").arg(mpClient->getHostName(), QString::number(mpClient->getHostPort()))));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Nick: %1").arg(mpClient->getNickName())));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Auto-Join Channels: %1").arg(mpClient->getChannels().join(" "))));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ This client supports Auto-Completion using the Tab key.")));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("$ Type <b>/help</b> for commands or <b>/help [command]</b> for command syntax.")));
    ircBrowser->append(qsl("\n"));

    mIrcStarted = true;
}

// For the IRC window's input, where typed commands are meant to run; see TIrcClient::sendText().
QPair<bool, QString> dlgIRC::sendMsg(const QString& target, const QString& message)
{
    const auto arguments = TIrcClient::validateMsgArguments(target, message);
    if (!arguments.first) {
        return arguments;
    }

    // inform the command parser of the target for this message.
    // parses the message and then reverts the target to avoid confusing our UI.
    const QString lastParserTarget = commandParser->target();
    commandParser->setTarget(target);
    IrcCommand* command = commandParser->parse(message);
    commandParser->setTarget(lastParserTarget);

    if (!command) {
        return {false, qsl("message could not be parsed")};
    }

    // we own the parsed command until sendCommand(), so early returns must free it
    const bool isCustomCommand = processCustomCommand(command);
    if (isCustomCommand) {
        delete command;
        return {true, QString()};
    }

    // read, and local echo built, before sendCommand() takes ownership (see TIrcClient::sendText())
    IrcConnection* connection = mpClient->connection();
    const IrcCommand::Type commandType = command->type();
    IrcMessage* msg = nullptr;
    if (commandType == IrcCommand::Message || commandType == IrcCommand::CtcpAction) {
        msg = command->toMessage(connection->nickName(), connection);
    }

    mpClient->sendCommand(command);

    // if the command was a quit command we should close the IRC window.
    if (commandType == IrcCommand::Quit) {
        setAttribute(Qt::WA_DeleteOnClose);
        close();
        return {true, QString()};
    }

    if (msg) {
        slot_showOwnMessage(msg);
        delete msg;
    }

    return {true, QString()};
}

void dlgIRC::slot_restarting(const QString& reason)
{
    ircBrowser->append(IrcMessageFormatter::formatMessage("! %1.").arg(reason));
}

void dlgIRC::slot_restarted()
{
    setClientWindowTitle();
}

void dlgIRC::setupCommandParser()
{
    // create a command parser and teach it some commands. notice also
    // that we must keep the command parser aware of the context in
    // setupBuffers() and onBufferActivated()
    commandParser = new IrcCommandParser(this);
    commandParser->setTolerant(true);
    commandParser->setTriggers(QStringList(qsl("/")));

    commandParser->addCommand(IrcCommand::CtcpAction, qsl("ACTION <target> <message...>"));
    commandParser->addCommand(IrcCommand::Admin, qsl("ADMIN (<server>)"));
    commandParser->addCommand(IrcCommand::Away, qsl("AWAY (<reason...>)"));
    commandParser->addCommand(IrcCommand::Info, qsl("INFO (<server>)"));
    commandParser->addCommand(IrcCommand::Invite, qsl("INVITE <user> (<#channel>)"));
    commandParser->addCommand(IrcCommand::Join, qsl("JOIN <#channel> (<key>)"));
    commandParser->addCommand(IrcCommand::Kick, qsl("KICK (<#channel>) <user> (<reason...>)"));
    commandParser->addCommand(IrcCommand::Knock, qsl("KNOCK <#channel> (<message...>)"));
    commandParser->addCommand(IrcCommand::List, qsl("LIST (<channels>) (<server>)"));
    commandParser->addCommand(IrcCommand::CtcpAction, qsl("ME [target] <message...>"));
    commandParser->addCommand(IrcCommand::Mode, qsl("MODE (<channel/user>) (<mode>) (<arg>)"));
    commandParser->addCommand(IrcCommand::Motd, qsl("MOTD (<server>)"));
    commandParser->addCommand(IrcCommand::Names, qsl("NAMES (<#channel>)"));
    commandParser->addCommand(IrcCommand::Nick, qsl("NICK <nick>"));
    commandParser->addCommand(IrcCommand::Notice, qsl("NOTICE <#channel/user> <message...>"));
    commandParser->addCommand(IrcCommand::Part, qsl("PART (<#channel>) (<message...>)"));
    commandParser->addCommand(IrcCommand::Ping, qsl("PING (<user>)"));
    commandParser->addCommand(IrcCommand::Quit, qsl("QUIT (<message...>)"));
    commandParser->addCommand(IrcCommand::Quote, qsl("QUOTE <command> (<parameters...>)"));
    commandParser->addCommand(IrcCommand::Stats, qsl("STATS <query> (<server>)"));
    commandParser->addCommand(IrcCommand::Time, qsl("TIME (<user>)"));
    commandParser->addCommand(IrcCommand::Topic, qsl("TOPIC (<#channel>) (<topic...>)"));
    commandParser->addCommand(IrcCommand::Trace, qsl("TRACE (<target>)"));
    commandParser->addCommand(IrcCommand::Users, qsl("USERS (<server>)"));
    commandParser->addCommand(IrcCommand::Version, qsl("VERSION (<user>)"));
    commandParser->addCommand(IrcCommand::Who, qsl("WHO <mask>"));
    commandParser->addCommand(IrcCommand::Whois, qsl("WHOIS <user>"));
    commandParser->addCommand(IrcCommand::Whowas, qsl("WHOWAS <user>"));

    commandParser->addCommand(IrcCommand::Custom, qsl("MSG <target> <message...>"));   // replaces the old /msg command.
    commandParser->addCommand(IrcCommand::Custom, qsl("CLEAR (<buffer>)"));            // clears the given buffer, or the current active if none are given.
    commandParser->addCommand(IrcCommand::Custom, qsl("CLOSE (<buffer>)"));            // closes the buffer and removes it from the list, uses current active buffer if none are given.
    commandParser->addCommand(IrcCommand::Custom, qsl("RECONNECT"));                   // Issues a Quit command and closes the IRC connection then reconnects to the IRC server.
    commandParser->addCommand(IrcCommand::Custom, qsl("HELP (<command>)"));            // displays some help information about a given command or lists all available commands.
    commandParser->addCommand(IrcCommand::Custom, qsl("MSGLIMIT <limit> (<buffer>)")); // sets buffer limit on all buffers and updates settings, or sets buffer limit on given buffer.
}

void dlgIRC::setupBuffers()
{
    IrcBufferModel* bufferModel = mpClient->bufferModel();
    connect(bufferModel, &IrcBufferModel::added, this, &dlgIRC::slot_onBufferAdded);
    connect(bufferModel, &IrcBufferModel::removed, this, &dlgIRC::slot_onBufferRemoved);
    bufferList->setModel(bufferModel);
    // keep the command parser aware of the context
    connect(bufferModel, &IrcBufferModel::channelsChanged, commandParser, &IrcCommandParser::setChannels);
    // keep track of the current buffer, see also onBufferActivated()
    connect(bufferList->selectionModel(), &QItemSelectionModel::currentChanged, this, &dlgIRC::slot_onBufferActivated);
    // the server buffer was added before there was anything here to see it
    for (IrcBuffer* buffer : bufferModel->buffers()) {
        slot_onBufferAdded(buffer);
    }
}

bool dlgIRC::processCustomCommand(IrcCommand* cmd)
{
    if (cmd->type() != IrcCommand::Custom || cmd->parameters().isEmpty()) {
        return false;
    }

    IrcBufferModel* bufferModel = mpClient->bufferModel();
    const QString cmdName = QString(cmd->parameters().at(0)).toUpper();
    if (cmdName == "CLEAR") {
        auto* buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
        if (cmd->parameters().count() > 1) {
            const QString bufferName = cmd->parameters().at(1);
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
        auto* buffer = bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
        if (cmd->parameters().count() > 1) {
            const QString bufferName = cmd->parameters().at(1);
            if (!bufferName.isEmpty()) {
                buffer = bufferModel->find(bufferName);
            }
        }
        if (buffer && buffer->title() != mpClient->serverBuffer()->title()) {
            // By the buffer, not by name: the server renames it once it says who it is
            bufferList->setCurrentIndex(bufferModel->index(mpClient->serverBuffer()));
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
        mpClient->restart();

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

        // the input line is cleared regardless, so a refusal must be reported
        const auto result = sendMsg(target, msgText);
        if (!result.first) {
            //: %1 is why the message could not be sent, e.g. 'no message given to send'
            const QString error = tr("[ERROR] Could not send that message: %1").arg(result.second);
            ircBrowser->append(IrcMessageFormatter::formatMessage(error, qsl("indianred")));
        }
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
            const QString error = tr("[Error] MSGLIMIT requires <limit> to be a whole number greater than zero!");
            ircBrowser->append(IrcMessageFormatter::formatMessage(error, qsl("indianred")));
            return true;
        }
        if (cmd->parameters().count() > 2) {
            const QString bufferName = cmd->parameters().at(2);
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
        help = tr("[HELP] Available Commands: %1").arg(commandParser->commands().join(qsl("  ")));
    } else {
        help = tr("[HELP] Syntax: %1").arg(commandParser->syntax(cmdName).replace(qsl("<"), qsl("&lt;")).replace(qsl(">"), qsl("&gt;")));
    }

    ircBrowser->append(IrcMessageFormatter::formatMessage(help));
}

void dlgIRC::slot_onConnected()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Connected to %1.")).arg(mpClient->getHostName()));
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Joining %1...")).arg(mpClient->getChannels().join(qsl(" "))));
}

void dlgIRC::slot_onConnecting()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Connecting %1...")).arg(mpClient->getHostName()));
}

void dlgIRC::slot_onDisconnected()
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! Disconnected from %1.")).arg(mpClient->getHostName()));
}

void dlgIRC::slot_onTextEdited()
{
    // clear the possible error indication
    lineEdit->setStyleSheet(QString());
}

void dlgIRC::slot_onTextEntered()
{
    const QString input = lineEdit->text();

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
        // owned here until sendCommand(), which a custom command never reaches
        const bool isCustomCommand = processCustomCommand(command);
        if (isCustomCommand) {
            delete command;
            lineEdit->clear();
            return;
        }

        IrcConnection* connection = mpClient->connection();
        const IrcCommand::Type commandType = command->type();
        IrcMessage* msg = nullptr;
        if (commandType == IrcCommand::Message || commandType == IrcCommand::CtcpAction) {
            msg = command->toMessage(connection->nickName(), connection);
        }

        // send to the server.
        mpClient->sendCommand(command);

        // if the command was a quit command we should close this window.
        if (commandType == IrcCommand::Quit) {
            setAttribute(Qt::WA_DeleteOnClose);
            close();
            return;
        }

        // echo own messages (servers do not send our own messages back)
        if (msg) {
            slot_showOwnMessage(msg);
            delete msg;
        }
        lineEdit->clear();
    } else if (input.length() > 1) {
        QString error;
        const QString command = lineEdit->text().mid(1).split(" ", Qt::SkipEmptyParts).value(0).toUpper();
        if (commandParser->commands().contains(command)) {
            error = tr("[ERROR] Syntax: %1").arg(commandParser->syntax(command).replace(qsl("<"), qsl("&lt;")).replace(qsl(">"), qsl("&gt;")));
        } else {
            error = tr("[ERROR] Unknown command: %1").arg(command);
        }
        ircBrowser->append(IrcMessageFormatter::formatMessage(error, qsl("indianred")));
        lineEdit->setStyleSheet(qsl("background: salmon"));
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

    if (mInputHistory.isEmpty()) {
        return;
    }

    lineEdit->setText(mInputHistory.at(mInputHistoryIdxCurrent));
    ++mInputHistoryIdxCurrent;
}

void dlgIRC::slot_onBufferAdded(IrcBuffer* buffer)
{
    // create a document for storing the buffer specific messages
    auto* document = new QTextDocument(buffer);
    document->setMaximumBlockCount(mMessageBufferLimit);
    bufferTexts.insert(buffer, document);
    // create a sorted model for buffer users
    auto* userModel = new IrcUserModel(buffer);
    userModel->setSortMethod(Irc::SortByTitle);
    userModels.insert(buffer, userModel);
    // activate the new buffer
    IrcBufferModel* bufferModel = mpClient->bufferModel();
    const int idx = bufferModel->buffers().indexOf(buffer);
    if (idx != -1) {
        bufferList->setCurrentIndex(bufferModel->index(idx));
    }
}

void dlgIRC::slot_onBufferRemoved(IrcBuffer* buffer)
{
    // the buffer specific models and documents are no longer needed
    delete userModels.take(buffer);
    delete bufferTexts.take(buffer);
}

void dlgIRC::slot_onBufferActivated(const QModelIndex& index)
{
    auto* buffer = index.data(Irc::BufferRole).value<IrcBuffer*>();
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
    auto* user = index.data(Irc::UserRole).value<IrcUser*>();
    if (user) {
        // ensure the "user" isn't our own client, can only do this by name.
        if (user->name() == mpClient->getNickName()) {
            return;
        }
        IrcBufferModel* bufferModel = mpClient->bufferModel();
        IrcBuffer* buffer = bufferModel->add(user->name());
        // activate the new query
        const int idx = bufferModel->buffers().indexOf(buffer);
        if (idx != -1) {
            bufferList->setCurrentIndex(bufferModel->index(idx));
        }
    }
}

// The on-screen document goes through the browser so it scrolls and repaints.
void dlgIRC::appendToDocument(QTextDocument* document, const QString& html)
{
    if (document == ircBrowser->document()) {
        ircBrowser->append(html);
        return;
    }

    QTextCursor cursor(document);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);
    if (!document->isEmpty()) {
        cursor.insertBlock();
    }
    cursor.insertHtml(html);
    cursor.endEditBlock();
}

// Our own lines, which the server does not echo back, go to the buffer on screen.
void dlgIRC::slot_showOwnMessage(IrcMessage* message)
{
    slot_showMessage(bufferList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>(), message);
}

void dlgIRC::slot_showMessage(IrcBuffer* buffer, IrcMessage* message)
{
    QTextDocument* document = bufferTexts.value(buffer);
    if (!document) {
        return;
    }
    const QString html = IrcMessageFormatter::formatMessage(message);
    if (html.isEmpty()) {
        return;
    }

    appendToDocument(document, html);

    // Being kicked makes IrcBufferModelPrivate::messageFilter() destroy the channel buffer,
    // so copy the line to the never-destroyed server buffer. The nick test mirrors that
    // filter's own destroy test: keep the two in step.
    const bool kickedUs = message->type() == IrcMessage::Kick && !static_cast<IrcKickMessage*>(message)->user().compare(mpClient->connection()->nickName(), Qt::CaseInsensitive);
    // a kick from a channel with no buffer already arrived on the server buffer (messageIgnored)
    IrcBuffer* serverBuffer = mpClient->serverBuffer();
    if (kickedUs && buffer != serverBuffer) {
        if (QTextDocument* serverDocument = bufferTexts.value(serverBuffer)) {
            appendToDocument(serverDocument, html);
        }
    }
}

void dlgIRC::slot_onAnchorClicked(const QUrl& link)
{
    QDesktopServices::openUrl(link);
}

void dlgIRC::slot_nickNameReserved(const QString& reserved, const QString& replacement)
{
    ircBrowser->append(IrcMessageFormatter::formatMessage(tr("! The Nickname %1 is reserved. Automatically changing Nickname to: %2").arg(reserved, replacement)));
}

void dlgIRC::showEvent(QShowEvent* event)
{
    startClient();
    event->ignore();
}

void dlgIRC::writeQSettings()
{
    if (auto* settings = MudletApp::getQSettings()) {
        settings->setValue("ircMessageBufferLimit", mMessageBufferLimit);
    }
}
