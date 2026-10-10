/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2017-2018, 2020, 2022, 2024 by Stephen Lyons            *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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


#include "TIrcClient.h"

#include "Host.h"
#include "MudletApp.h"
#include "ircmessageformatter.h"

#include <IrcBuffer>
#include <IrcBufferModel>
#include <IrcCommand>
#include <IrcConnection>

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QSaveFile>

#include <algorithm>

// The translated strings here keep the "dlgIRC" context they were translated in.

TIrcClient::TIrcClient(Host* pHost)
: QObject(pHost)
, mpHost(pHost)
, mRealName(MudletApp::scmVersion())
{
    mpConnection = new IrcConnection(this);
    mpConnection->setReconnectDelay(5);

    connect(mpConnection, &IrcConnection::nickNameRequired, this, &TIrcClient::slot_nickNameRequired);
    connect(mpConnection, &IrcConnection::nickNameChanged, this, &TIrcClient::slot_nickNameChanged);
    connect(mpConnection, &IrcConnection::joinMessageReceived, this, &TIrcClient::slot_joinedChannel);
    connect(mpConnection, &IrcConnection::partMessageReceived, this, &TIrcClient::slot_partedChannel);
    connect(mpConnection, &IrcConnection::numericMessageReceived, this, &TIrcClient::slot_receiveNumericMessage);
    // Only the server's RPL_YOURHOST names it again, so a dropped connection must not keep answering with the old name
    connect(mpConnection, &IrcConnection::disconnected, this, [this]() {
        mConnectedHostName.clear();
    });

    mPassword = readIrcPassword(mpHost);
    mHostName = readIrcHostName(mpHost);
    mHostPort = readIrcHostPort(mpHost);
    mHostSecure = readIrcHostSecure(mpHost);
    mNickName = readIrcNickName(mpHost);
    mChannels = readIrcChannels(mpHost);

    mpConnection->setNickName(mNickName);
    mpConnection->setUserName(mUserName);
    mpConnection->setPassword(mPassword);
    mpConnection->setRealName(mRealName);
    mpConnection->setHost(mHostName);
    mpConnection->setPort(mHostPort);
    mpConnection->setSecure(mHostSecure);
}

TIrcClient::~TIrcClient()
{
    if (mpConnection->isActive()) {
        const QString quitMsg = QCoreApplication::translate("dlgIRC", "%1 closed their client.").arg(mNickName);
        mpConnection->quit(quitMsg);
        mpConnection->close();
    }
}

// Who a line was sent to, for Lua: a notice or a message names its target, anything else is
// reported against the buffer it arrived in.
static QString messageTarget(IrcMessage* msg, const QString& bufferName)
{
    QString target = bufferName;
    switch (msg->type()) {
    case IrcMessage::Notice: {
        auto* msgNotice = static_cast<IrcNoticeMessage*>(msg);
        target = msgNotice->target();
        break;
    }
    case IrcMessage::Private: {
        auto* msgPrivate = static_cast<IrcPrivateMessage*>(msg);
        target = msgPrivate->target();
        break;
    }
    default:
        // Other message types are not expected - I hope - SlySven
        qWarning().noquote().nospace() << "TIrcClient messageTarget(..., \"" << bufferName << "\") WARNING - message of type: " << msg->type()
                                       << " not explicitly handled, this needs fixing by Mudlet Makers...";
    }
    return target;
}

void TIrcClient::start()
{
    if (mStarted) {
        return;
    }

    mpConnection->sendCommand(IrcCommand::createJoin(mChannels));
    mpConnection->open();
    mStarted = true;

    mpBufferModel = new IrcBufferModel(mpConnection);
    connect(mpBufferModel, &IrcBufferModel::added, this, &TIrcClient::slot_bufferAdded);
    mpServerBuffer = mpBufferModel->add(mpConnection->host());
    mpServerBuffer->setName(mpConnection->host());
    connect(mpBufferModel, &IrcBufferModel::messageIgnored, mpServerBuffer, &IrcBuffer::receiveMessage);
}

IrcBuffer* TIrcClient::serverBuffer() const
{
    return mpServerBuffer;
}

void TIrcClient::slot_bufferAdded(IrcBuffer* buffer)
{
    connect(buffer, &IrcBuffer::messageReceived, this, [this, buffer](IrcMessage* message) {
        receiveBufferMessage(buffer, message);
    });
}

void TIrcClient::receiveBufferMessage(IrcBuffer* buffer, IrcMessage* message)
{
    // the reply to a typed PING is formatted from its timestamp
    if (message->type() == IrcMessage::Pong && mPingStarted) {
        message->setTimeStamp(QDateTime::fromMSecsSinceEpoch(mPingStarted));
        mPingStarted = 0;
    }

    // a plain-text copy for Lua, as long as it isn't our own
    if (!message->isOwn() && mpHost) {
        const QString textToLua = IrcMessageFormatter::formatMessage(message, true);
        if (!textToLua.isEmpty()) {
            mpHost->postIrcMessage(message->nick(), messageTarget(message, buffer->title()), textToLua);
        }
    }

    emit signal_messageReceived(buffer, message);
}

// CR or LF ends an IRC command; NUL is forbidden in one.
static bool textBreaksIrcLine(const QString& text)
{
    return std::any_of(text.cbegin(), text.cend(), [](const QChar character) {
        return character == QChar::CarriageReturn || character == QChar::LineFeed || character == QChar::Null;
    });
}

// A nick or channel is a single IRC parameter, which ends at the first space.
static bool textHasSpace(const QString& text)
{
    return std::any_of(text.cbegin(), text.cend(), [](const QChar character) {
        return character.isSpace();
    });
}

// Quoted text in a refusal must not break its line, and a game server can make it arbitrarily long.
static QString escapedForError(const QString& text)
{
    QString escaped = text;
    escaped.replace(QChar::Null, qsl("\\0")).replace(QChar::CarriageReturn, qsl("\\r")).replace(QChar::LineFeed, qsl("\\n"));
    if (escaped.length() > 40) {
        escaped.truncate(40);
        escaped.append(qsl("..."));
    }
    return escaped;
}

// Parameters end at a space and commands at CR LF, so either in an argument would let the server read
// a second command (a QUIT, a PRIVMSG elsewhere) out of one send - and callers often relay game text.
// Refused rather than stripped, so the caller knows and can split the text itself. IRC formatting
// codes (bold, colour, CTCP delimiter) are legitimate and left alone.
QPair<bool, QString> TIrcClient::validateMsgArguments(const QString& target, const QString& message)
{
    if (target.isEmpty()) {
        return {false, qsl("no target given, name the channel or the nick to send the message to")};
    }
    if (textBreaksIrcLine(target)) {
        return {false, qsl("target \"%1\" must not contain a line break or a null character").arg(escapedForError(target))};
    }
    // a comma-separated target list is still one PRIVMSG, so allowed if each name is valid
    const QStringList names = target.split(QLatin1Char(','));
    for (const QString& name : names) {
        if (name.isEmpty()) {
            return {false, qsl("target \"%1\" has an empty name in its list").arg(escapedForError(target))};
        }
        if (textHasSpace(name)) {
            return {false, qsl("target \"%1\" must be a channel or a nick name, which holds no spaces").arg(escapedForError(name))};
        }
        if (name.startsWith(QLatin1Char(':'))) {
            return {false, qsl("target \"%1\" must not start with a colon").arg(escapedForError(name))};
        }
    }
    if (message.isEmpty()) {
        return {false, qsl("no message given to send")};
    }
    if (textBreaksIrcLine(message)) {
        return {false, qsl("message \"%1\" must not contain a line break or a null character").arg(escapedForError(message))};
    }
    return {true, QString()};
}

// For Lua's sendIrc(): sends text, never parses commands. The IRC window's parser turns a leading "/"
// into any verb at all (via QUOTE or raw relay of unknown verbs), which would let game text relayed
// by a script pick the command without needing a CR or LF.
QPair<bool, QString> TIrcClient::sendText(const QString& target, const QString& message)
{
    const auto arguments = validateMsgArguments(target, message);
    if (!arguments.first) {
        return arguments;
    }

    IrcCommand* command = IrcCommand::createMessage(target, message);
    // built first because Communi says a parentless command is unsafe to touch once sendCommand() owns it
    IrcMessage* msg = command->toMessage(mpConnection->nickName(), mpConnection);
    mpConnection->sendCommand(command);
    emit signal_messageSent(msg);
    delete msg;

    return {true, QString()};
}

void TIrcClient::sendCommand(IrcCommand* command)
{
    if (command->type() == IrcCommand::Ping) {
        mPingStarted = QDateTime::currentMSecsSinceEpoch();
    }

    mpConnection->sendCommand(command);
}

bool TIrcClient::restart(bool reloadConfigs)
{
    // a session that was never started stays closed
    if (!mStarted) {
        return false;
    }

    const QString reason = QCoreApplication::translate("dlgIRC", "Restarting IRC Client");
    emit signal_restarting(reason);

    // the channels' buffers go, the server's stays.
    if (mpBufferModel) {
        for (const QString& chName : getChannels()) {
            if (mpServerBuffer && chName == mpServerBuffer->name()) {
                continue;
            }
            mpBufferModel->remove(chName);
        }
    }

    // issue a quit message to the network if we're connected.
    if (mpConnection->isConnected()) {
        mpConnection->quit(reason);
    }

    mpConnection->close();

    if (reloadConfigs) {
        mHostName = readIrcHostName(mpHost);
        mHostPort = readIrcHostPort(mpHost);
        mHostSecure = readIrcHostSecure(mpHost);
        mNickName = readIrcNickName(mpHost);
        mChannels = readIrcChannels(mpHost);
        mPassword = readIrcPassword(mpHost);

        mpConnection->setNickName(mNickName);
        mpConnection->setHost(mHostName);
        mpConnection->setPort(mHostPort);
        mpConnection->setSecure(mHostSecure);
        mpConnection->setPassword(mPassword);
    }

    // queue auto-joined channels and reopen the connection.
    mpConnection->sendCommand(IrcCommand::createJoin(mChannels));
    mpConnection->open();

    if (mpServerBuffer) {
        mpServerBuffer->setName(mpConnection->host());
    }
    emit signal_restarted();
    return true;
}

void TIrcClient::slot_nickNameRequired(const QString& reserved, QString* alt)
{
    Q_UNUSED(alt)
    const QString newNick = qsl("%1_%2").arg(reserved, QString::number(rand() % 10000));
    emit signal_nickNameReserved(reserved, newNick);
    mpConnection->setNickName(newNick);
}

void TIrcClient::slot_nickNameChanged(const QString& nick)
{
    if (!mpHost || nick == mNickName) {
        return;
    }

    // send a notice to Lua about the nick name change.
    mpHost->postIrcMessage(mNickName, nick, QCoreApplication::translate("dlgIRC", "Your nick has changed."));
    mNickName = nick;

    emit signal_nickNameChanged();
}

void TIrcClient::slot_joinedChannel(IrcJoinMessage* message)
{
    if (!mpHost) {
        return;
    }

    if (!mReadyForSending) {
        mReadyForSending = true;
    }

    const QString chan = message->channel();
    if (!mChannels.contains(chan)) {
        mChannels << chan;
    }

    if (message->isOwn()) {
        const QString luaText = IrcMessageFormatter::formatMessage(static_cast<IrcMessage*>(message), true);
        mpHost->postIrcMessage(message->nick(), message->channel(), luaText);
    }
}

void TIrcClient::slot_partedChannel(IrcPartMessage* message)
{
    if (!mpHost) {
        return;
    }

    const QString chan = message->channel();
    if (mChannels.contains(chan)) {
        mChannels.removeAll(chan);
    }

    if (message->isOwn()) {
        const QString luaText = IrcMessageFormatter::formatMessage(static_cast<IrcMessage*>(message), true);
        mpHost->postIrcMessage(message->nick(), message->channel(), luaText);
    }
}

void TIrcClient::slot_receiveNumericMessage(IrcNumericMessage* message)
{
    if (message->code() == Irc::RPL_YOURHOST) {
        mConnectedHostName = message->nick();
        if (mpServerBuffer) {
            mpServerBuffer->setName(mConnectedHostName);
        }
    }
}

QString TIrcClient::readIrcHostName(Host* pH)
{
    QString hostname = pH->readProfileData(TIrcClient::HostNameCfgItem);
    if (hostname.isEmpty()) {
        hostname = TIrcClient::DefaultHostName;
    }
    return hostname;
}

int TIrcClient::readIrcHostPort(Host* pH)
{
    const QString portStr = pH->readProfileData(TIrcClient::HostPortCfgItem);
    bool ok;
    int port = portStr.toInt(&ok);
    if (portStr.isEmpty() || !ok) {
        port = TIrcClient::DefaultHostPort;
    } else if (port > 65535 || port < 1) {
        port = TIrcClient::DefaultHostPort;
    }
    return port;
}

bool TIrcClient::readIrcHostSecure(Host* pH)
{
    const QString secureStr = pH->readProfileData(TIrcClient::HostSecureCfgItem);
    return secureStr.contains(QLatin1String("true"), Qt::CaseInsensitive);
}

QString TIrcClient::readIrcNickName(Host* pH)
{
    QString nick = pH->readProfileData(TIrcClient::NickNameCfgItem);
    if (nick.isEmpty()) {
        // if the new config doesn't exist, try loading the old one.
        nick = readAppDefaultIrcNick();

        if (nick.isEmpty()) {
            nick = qsl("%1%2").arg(TIrcClient::DefaultNickName, QString::number(rand() % 10000));
        }
    }
    return nick;
}

QString TIrcClient::readIrcPassword(Host* pH)
{
    QString pass = pH->readProfileData(TIrcClient::PasswordCfgItem);
    return pass;
}

QString TIrcClient::readAppDefaultIrcNick()
{
    QFile file(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("irc_nick")));
    const bool opened = file.open(QIODevice::ReadOnly);
    QString rstr;
    if (opened) {
        QDataStream ifs(&file);
        ifs.setVersion(QDataStream::Qt_5_12);
        ifs >> rstr;
        file.close();
    }
    return rstr;
}

void TIrcClient::writeAppDefaultIrcNick(const QString& nick)
{
    QSaveFile file(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("irc_nick")));
    const bool opened = file.open(QIODevice::WriteOnly);
    if (opened) {
        QDataStream ofs(&file);
        ofs.setVersion(QDataStream::Qt_5_12);
        ofs << nick;
        if (!file.commit()) {
            qDebug() << "TIrcClient::writeAppDefaultIrcNick: error saving default nickname: " << file.errorString();
        }
    }
}

QStringList TIrcClient::readIrcChannels(Host* pH)
{
    QStringList channels;
    const QString channelstr = pH->readProfileData(TIrcClient::ChannelsCfgItem);
    if (channelstr.isEmpty()) {
        channels << TIrcClient::DefaultChannels;
    } else {
        // a profile saved before writeIrcChannels() refused them can still hold unusable names
        for (const QString& channel : channelstr.split(qsl(" "), Qt::SkipEmptyParts)) {
            if (validIrcChannelName(channel)) {
                channels << channel;
            }
        }
        if (channels.isEmpty()) {
            channels << TIrcClient::DefaultChannels;
        }
    }
    return channels;
}

// The readers put a default in place of an empty, out of range or unusable value, so
// the writers below refuse one rather than report a setting that will never be used.
QPair<bool, QString> TIrcClient::writeIrcHostName(Host* pH, const QString& hostname)
{
    if (hostname.isEmpty()) {
        return {false, qsl("hostname must not be empty")};
    }
    if (textHasSpace(hostname) || textBreaksIrcLine(hostname)) {
        return {false, qsl("hostname \"%1\" must not hold a space or a line break").arg(escapedForError(hostname))};
    }

    return pH->writeProfileData(TIrcClient::HostNameCfgItem, hostname);
}

QPair<bool, QString> TIrcClient::writeIrcHostPort(Host* pH, int port)
{
    if (port < 1 || port > 65535) {
        return {false, qsl("invalid port number %1 given, it must be in range 1 to 65535").arg(port)};
    }

    return pH->writeProfileData(TIrcClient::HostPortCfgItem, QString::number(port));
}

QPair<bool, QString> TIrcClient::writeIrcHostSecure(Host* pH, bool secure)
{
    return pH->writeProfileData(TIrcClient::HostSecureCfgItem, (secure ? QLatin1String("true") : QLatin1String("false")));
}

QPair<bool, QString> TIrcClient::writeIrcNickName(Host* pH, const QString& nickname)
{
    if (nickname.isEmpty()) {
        return {false, qsl("nick must not be empty")};
    }
    // Sent as "NICK <nickname>" at registration, bypassing validateMsgArguments(); IrcConnection
    // takes only the first word, but a line break within it would start an injected command.
    if (textBreaksIrcLine(nickname) || textHasSpace(nickname)) {
        return {false, qsl("nick name \"%1\" must be a single word, without a line break or a null character").arg(escapedForError(nickname))};
    }

    // update app-wide file to set a default nick as whatever the last-used nick was.
    writeAppDefaultIrcNick(nickname);

    return pH->writeProfileData(TIrcClient::NickNameCfgItem, nickname);
}

QPair<bool, QString> TIrcClient::validateIrcPassword(const QString& password)
{
    // As for the nick, but as the trailing parameter of "PASS :<password>" spaces are fine.
    // Never quote the password back.
    if (textBreaksIrcLine(password)) {
        return {false, qsl("password must not contain a line break or a null character")};
    }

    return {true, QString()};
}

QPair<bool, QString> TIrcClient::writeIrcPassword(Host* pH, const QString& password)
{
    const QPair<bool, QString> valid = validateIrcPassword(password);
    if (!valid.first) {
        return valid;
    }

    return pH->writeProfileData(TIrcClient::PasswordCfgItem, password);
}

// The stored list is space-joined and the JOIN command comma-joined, so a name holding
// either would come back as two channels.
bool TIrcClient::validIrcChannelName(const QString& channel)
{
    if (!channel.startsWith(QLatin1Char('#')) && !channel.startsWith(QLatin1Char('&')) && !channel.startsWith(QLatin1Char('+'))) {
        return false;
    }
    return !textHasSpace(channel) && !textBreaksIrcLine(channel) && !channel.contains(QLatin1Char(','));
}

QPair<bool, QString> TIrcClient::writeIrcChannels(Host* pH, const QStringList& channels)
{
    if (channels.isEmpty()) {
        return {false, qsl("no (valid) channel names provided")};
    }
    for (const QString& channel : channels) {
        if (!validIrcChannelName(channel)) {
            return {false, qsl("channel name \"%1\" must start with #, & or + and hold no space or comma").arg(escapedForError(channel))};
        }
    }

    return pH->writeProfileData(TIrcClient::ChannelsCfgItem, channels.join(qsl(" ")));
}
