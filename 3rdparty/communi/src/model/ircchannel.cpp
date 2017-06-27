/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ircchannel.h"
#include "ircchannel_p.h"
#include "ircusermodel.h"
#include "ircusermodel_p.h"
#include "ircbuffermodel.h"
#include "ircbuffermodel_p.h"
#include "ircconnection.h"
#include "ircnetwork.h"
#include "irccommand.h"
#include "ircuser_p.h"
#include "irc.h"

IRC_BEGIN_NAMESPACE

/*!
    \file ircchannel.h
    \brief \#include &lt;IrcChannel&gt;
 */

/*!
    \class IrcChannel ircchannel.h <IrcChannel>
    \ingroup models
    \brief Keeps track of channel status.

    \sa IrcBufferModel
*/

#ifndef IRC_DOXYGEN
static QString getPrefix(const QString& name, const QStringList& prefixes)
{
    int i = 0;
    while (i < name.length() && prefixes.contains(name.at(i)))
        ++i;
    return name.left(i);
}

static QString getMode(IrcNetwork *network, const QString& prefix)
{
    QString mode;
    foreach (const QString& p, prefix)
        mode += network->prefixToMode(p);
    return mode;
}

static QString channelName(const QString& title, const QStringList& prefixes)
{
    int i = 0;
    while (i < title.length() && prefixes.contains(title.at(i)))
        ++i;
    return title.mid(i);
}

static QString userName(const QString& name, const QStringList& prefixes)
{
    QString copy = name;
    while (!copy.isEmpty() && prefixes.contains(copy.at(0)))
        copy.remove(0, 1);
    return Irc::nickFromPrefix(copy);
}

IrcChannelPrivate::IrcChannelPrivate() : active(false), enabled(true)
{
    qRegisterMetaType<IrcChannel*>();
    qRegisterMetaType<QList<IrcChannel*> >();
}

IrcChannelPrivate::~IrcChannelPrivate()
{
}

void IrcChannelPrivate::init(const QString& title, IrcBufferModel* m)
{
    IrcBufferPrivate::init(title, m);

    const QStringList chanTypes = m->network()->channelTypes();
    prefix = getPrefix(title, chanTypes);
    name = channelName(title, chanTypes);
}

void IrcChannelPrivate::connected()
{
    // not active until joined
    setActive(false);
}

void IrcChannelPrivate::disconnected()
{
    setActive(false);
}

void IrcChannelPrivate::setActive(bool value)
{
    Q_Q(IrcChannel);
    if (active != value) {
        active = value;
        emit q->activeChanged(active);
    }
}

void IrcChannelPrivate::changeModes(const QString& value, const QStringList& arguments)
{
    Q_Q(IrcChannel);
    const IrcNetwork* network = q->network();

    QMap<QString, QString> ms = modes;
    QStringList args = arguments;

    bool add = true;
    for (int i = 0; i < value.size(); ++i) {
        const QString m = value.at(i);
        if (m == QLatin1String("+")) {
            add = true;
        } else if (m == QLatin1String("-")) {
            add = false;
        } else {
            if (add) {
                QString a;
                if (!args.isEmpty() && network && network->channelModes(IrcNetwork::TypeB | IrcNetwork::TypeC).contains(m))
                    a = args.takeFirst();
                ms.insert(m, a);
            } else {
                ms.remove(m);
            }
        }
    }

    if (modes != ms) {
        setKey(ms.value(QLatin1String("k")));
        modes = ms;
        emit q->modeChanged(q->mode());
    }
}

void IrcChannelPrivate::setModes(const QString& value, const QStringList& arguments)
{
    Q_Q(IrcChannel);
    const IrcNetwork* network = q->network();

    QMap<QString, QString> ms;
    QStringList args = arguments;

    for (int i = 0; i < value.size(); ++i) {
        const QString m = value.at(i);
        if (m != QLatin1String("+") && m != QLatin1String("-")) {
            QString a;
            if (!args.isEmpty() && network && network->channelModes(IrcNetwork::TypeB | IrcNetwork::TypeC).contains(m))
                a = args.takeFirst();
            ms.insert(m, a);
        }
    }

    if (modes != ms) {
        setKey(ms.value(QLatin1String("k")));
        modes = ms;
        emit q->modeChanged(q->mode());
    }
}

void IrcChannelPrivate::setTopic(const QString& value)
{
    Q_Q(IrcChannel);
    if (topic != value) {
        topic = value;
        emit q->topicChanged(topic);
    }
}

void IrcChannelPrivate::setKey(const QString& value)
{
    Q_Q(IrcChannel);
    if (modes.value(QLatin1String("k")) != value) {
        modes.insert(QLatin1String("k"), value);
        emit q->keyChanged(value);
    }
}

void IrcChannelPrivate::addUser(const QString& name)
{
    Q_Q(IrcChannel);
    const QStringList prefixes = q->network()->prefixes();

    IrcUser* user = new IrcUser(q);
    IrcUserPrivate* priv = IrcUserPrivate::get(user);
    priv->channel = q;
    priv->setName(userName(name, prefixes));
    priv->setPrefix(getPrefix(name, prefixes));
    priv->setMode(getMode(q->network(), user->prefix()));
    activeUsers.prepend(user);
    userList.append(user);
    userMap.insert(user->name(), user);
    names = userMap.keys();

    foreach (IrcUserModel* model, userModels)
        IrcUserModelPrivate::get(model)->addUser(user);
}

bool IrcChannelPrivate::removeUser(const QString& name)
{
    if (IrcUser* user = userMap.value(name)) {
        userMap.remove(name);
        names = userMap.keys();
        userList.removeOne(user);
        activeUsers.removeOne(user);
        foreach (IrcUserModel* model, userModels)
            IrcUserModelPrivate::get(model)->removeUser(user);
        user->deleteLater();
        return true;
    }
    return false;
}

void IrcChannelPrivate::setUsers(const QStringList& users)
{
    Q_Q(IrcChannel);
    const QStringList prefixes = q->network()->prefixes();

    qDeleteAll(userList);
    userMap.clear();
    userList.clear();
    activeUsers.clear();

    foreach (const QString& name, users) {
        IrcUser* user = new IrcUser(q);
        IrcUserPrivate* priv = IrcUserPrivate::get(user);
        priv->channel = q;
        priv->setName(userName(name, prefixes));
        priv->setPrefix(getPrefix(name, prefixes));
        priv->setMode(getMode(q->network(), user->prefix()));
        activeUsers.append(user);
        userList.append(user);
        userMap.insert(user->name(), user);
    }
    names = userMap.keys();

    foreach (IrcUserModel* model, userModels)
        IrcUserModelPrivate::get(model)->setUsers(userList);
}

bool IrcChannelPrivate::renameUser(const QString& from, const QString& to)
{
    if (IrcUser* user = userMap.take(from)) {
        IrcUserPrivate::get(user)->setName(to);
        userMap.insert(to, user);
        names = userMap.keys();

        foreach (IrcUserModel* model, userModels) {
            IrcUserModelPrivate::get(model)->renameUser(user);
            emit model->namesChanged(names);
        }
        return true;
    }
    return false;
}

void IrcChannelPrivate::setUserMode(const QString& name, const QString& command)
{
    if (IrcUser* user = userMap.value(name)) {
        bool add = true;
        QString mode = user->mode();
        QString prefix = user->prefix();
        const IrcNetwork* network = model->network();
        for (int i = 0; i < command.size(); ++i) {
            QChar c = command.at(i);
            if (c == QLatin1Char('+')) {
                add = true;
            } else if (c == QLatin1Char('-')) {
                add = false;
            } else {
                QString p = network->modeToPrefix(c);
                if (add) {
                    if (!mode.contains(c))
                        mode += c;
                    if (!prefix.contains(p))
                        prefix += p;
                } else {
                    mode.remove(c);
                    prefix.remove(p);
                }
            }
        }

        QString sortedMode;
        foreach (const QString& m, network->modes())
            if (mode.contains(m))
                sortedMode += m;

        QString sortedPrefix;
        foreach (const QString& p, network->prefixes())
            if (prefix.contains(p))
                sortedPrefix += p;

        IrcUserPrivate* priv = IrcUserPrivate::get(user);
        priv->setPrefix(sortedPrefix);
        priv->setMode(sortedMode);

        foreach (IrcUserModel* model, userModels)
            IrcUserModelPrivate::get(model)->setUserMode(user);
    }
}

void IrcChannelPrivate::promoteUser(const QString& name)
{
    if (IrcUser* user = userMap.value(name)) {
        const int idx = activeUsers.indexOf(user);
        Q_ASSERT(idx != -1);
        activeUsers.move(idx, 0);
        foreach (IrcUserModel* model, userModels)
            IrcUserModelPrivate::get(model)->promoteUser(user);
    }
}

bool IrcChannelPrivate::setUserAway(const QString& name, bool away)
{
    if (IrcUser* user = userMap.value(name)) {
        IrcUserPrivate* priv = IrcUserPrivate::get(user);
        priv->setAway(away);
        foreach (IrcUserModel* model, userModels)
            IrcUserModelPrivate::get(model)->updateUser(user);
        return true;
    }
    return false;
}

void IrcChannelPrivate::setUserServOp(const QString& name, bool servOp)
{
    if (IrcUser* user = userMap.value(name)) {
        IrcUserPrivate* priv = IrcUserPrivate::get(user);
        priv->setServOp(servOp);
        foreach (IrcUserModel* model, userModels)
            IrcUserModelPrivate::get(model)->updateUser(user);
    }
}

bool IrcChannelPrivate::processAwayMessage(IrcAwayMessage* message)
{
    setUserAway(message->nick(), message->isAway());
    return false;
}

bool IrcChannelPrivate::processJoinMessage(IrcJoinMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback)) {
        if (message->isOwn()) {
            setActive(true);
            enabled = true;
        } else {
            addUser(message->nick());
        }
    }
    return true;
}

bool IrcChannelPrivate::processKickMessage(IrcKickMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback)) {
        if (!message->user().compare(message->connection()->nickName(), Qt::CaseInsensitive)) {
            setActive(false);
            enabled = false;
            return true;
        }
        return removeUser(message->user());
    }
    return userMap.contains(message->user());
}

bool IrcChannelPrivate::processModeMessage(IrcModeMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback)) {
        if (message->kind() == IrcModeMessage::Channel) {
            if (message->isReply())
                setModes(message->mode(), message->arguments());
            else
                changeModes(message->mode(), message->arguments());
            return true;
        } else if (!message->argument().isEmpty()) {
            setUserMode(message->argument(), message->mode());
        }
    }
    return true;
}

bool IrcChannelPrivate::processNamesMessage(IrcNamesMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback))
        setUsers(message->names());
    return true;
}

bool IrcChannelPrivate::processNickMessage(IrcNickMessage* message)
{
    const bool renamed = renameUser(message->oldNick(), message->newNick());
    if (renamed)
        promoteUser(message->newNick());
    return renamed;
}

bool IrcChannelPrivate::processNoticeMessage(IrcNoticeMessage* message)
{
    promoteUser(message->nick());
    return false;
}

bool IrcChannelPrivate::processNumericMessage(IrcNumericMessage* message)
{
    promoteUser(message->nick());
    return message->isImplicit();
}

bool IrcChannelPrivate::processPartMessage(IrcPartMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback)) {
        if (message->isOwn()) {
            setActive(false);
            enabled = false;
            return true;
        }
        return removeUser(message->nick());
    }
    return true;
}

bool IrcChannelPrivate::processPrivateMessage(IrcPrivateMessage* message)
{
    const QString content = message->content();
    const bool prefixed = !content.isEmpty() && message->network()->prefixes().contains(content.at(0));
    foreach (IrcUser* user, activeUsers) {
        const QString str = prefixed ? user->title() : user->name();
        if (content.startsWith(str)) {
            promoteUser(user->name());
            break;
        }
    }
    promoteUser(message->nick());
    return true;
}

bool IrcChannelPrivate::processQuitMessage(IrcQuitMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback)) {
        if (message->isOwn()) {
            setActive(false);
            return true;
        }
        return removeUser(message->nick()) || IrcBufferPrivate::processQuitMessage(message);
    }
    return userMap.contains(message->nick()) || IrcBufferPrivate::processQuitMessage(message);
}

bool IrcChannelPrivate::processTopicMessage(IrcTopicMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback))
        setTopic(message->topic());
    return true;
}

bool IrcChannelPrivate::processWhoReplyMessage(IrcWhoReplyMessage *message)
{
    if (message->isValid()) {
        setUserAway(message->nick(), message->isAway());
        setUserServOp(message->nick(), message->isServOp());
    }
    return message->isImplicit();
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new channel object with \a parent.
 */
IrcChannel::IrcChannel(QObject* parent)
    : IrcBuffer(*new IrcChannelPrivate, parent)
{
}

/*!
    Destructs the channel object.
 */
IrcChannel::~IrcChannel()
{
    Q_D(IrcChannel);
    qDeleteAll(d->userList);
    d->userList.clear();
    d->userMap.clear();
    d->names.clear();
    d->userModels.clear();
    emit destroyed(this);
}

/*!
    \since 3.1

    This property holds the channel key.

    \par Access function:
    \li QString <b>key</b>() const

    \par Notifier signal:
    \li void <b>keyChanged</b>(const QString& key)
 */
QString IrcChannel::key() const
{
    Q_D(const IrcChannel);
    return d->modes.value(QLatin1String("k"));
}

/*!
    This property holds the complete channel mode including possible arguments.

    \par Access function:
    \li QString <b>mode</b>() const

    \par Notifier signal:
    \li void <b>modeChanged</b>(const QString& mode)
 */
QString IrcChannel::mode() const
{
    Q_D(const IrcChannel);
    QString m = QStringList(d->modes.keys()).join(QString());
    QStringList a = d->modes.values();
    a.removeAll(QString());
    if (!a.isEmpty())
        m += QLatin1String(" ") + a.join(QLatin1String(" "));
    if (!m.isEmpty())
        m.prepend(QLatin1String("+"));
    return m;
}

/*!
    This property holds the channel topic.

    \par Access function:
    \li QString <b>topic</b>() const

    \par Notifier signal:
    \li void <b>topicChanged</b>(const QString& topic)
 */
QString IrcChannel::topic() const
{
    Q_D(const IrcChannel);
    return d->topic;
}

bool IrcChannel::isActive() const
{
    Q_D(const IrcChannel);
    return IrcBuffer::isActive() && d->active;
}

/*!
    \since 3.3

    Sends a who command to the channel.

    This method is provided for convenience. It is equal to:
    \code
    IrcCommand* command = IrcCommand::createWho(channel->title());
    channel->sendCommand(command);
    \endcode

    \sa IrcBuffer::sendCommand(), IrcCommand::createWho()
 */
void IrcChannel::who()
{
    sendCommand(IrcCommand::createWho(title()));
}

/*!
    \since 3.1

    Joins the channel with an optional \a key.

    This method is provided for convenience. It is equal to:
    \code
    IrcCommand* command = IrcCommand::createJoin(channel->title(), key);
    channel->sendCommand(command);
    \endcode

    \sa IrcBuffer::sendCommand(), IrcCommand::createJoin()
 */
void IrcChannel::join(const QString& key)
{
    Q_D(IrcChannel);
    if (!key.isEmpty())
        d->setKey(key);
    d->enabled = true;
    sendCommand(IrcCommand::createJoin(title(), IrcChannel::key()));
}

/*!
    Parts the channel with an optional \a reason.

    This method is provided for convenience. It is equal to:
    \code
    IrcCommand* command = IrcCommand::createPart(channel->title(), reason);
    channel->sendCommand(command);
    \endcode

    \sa IrcBuffer::sendCommand(), IrcCommand::createPart()
 */
void IrcChannel::part(const QString& reason)
{
    Q_D(IrcChannel);
    d->enabled = false;
    sendCommand(IrcCommand::createPart(title(), reason));
}

/*!
    \since 3.1

    Closes the channel with an optional \a reason.

    \sa IrcBuffer::close(), IrcChannel::part()
 */
void IrcChannel::close(const QString& reason)
{
    Q_D(IrcChannel);
    d->enabled = false;
    if (isActive())
        part(reason);
    IrcBuffer::close(reason);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const IrcChannel* channel)
{
    if (!channel)
        return debug << "IrcChannel(0x0) ";
    debug.nospace() << channel->metaObject()->className() << '(' << (void*) channel;
    if (!channel->objectName().isEmpty())
        debug.nospace() << ", name=" << qPrintable(channel->objectName());
    if (!channel->title().isEmpty())
        debug.nospace() << ", title=" << qPrintable(channel->title());
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircchannel.cpp"

IRC_END_NAMESPACE
