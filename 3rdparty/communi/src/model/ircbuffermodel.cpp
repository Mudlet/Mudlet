/*
  Copyright (C) 2008-2020 The Communi Project

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

#include "ircbuffermodel.h"
#include "ircbuffermodel_p.h"
#include "ircchannel_p.h"
#include "ircbuffer_p.h"
#include "ircnetwork.h"
#include "ircchannel.h"
#include "ircmessage.h"
#include "irccommand.h"
#include "ircconnection.h"
#include <qmetatype.h>
#include <qmetaobject.h>
#include <qdatastream.h>
#include <qvariant.h>
#include <qtimer.h>
#include <algorithm>

IRC_BEGIN_NAMESPACE

/*!
    \file ircbuffermodel.h
    \brief \#include &lt;IrcBufferModel&gt;
 */

/*!
    \class IrcBufferModel ircbuffermodel.h <IrcBufferModel>
    \ingroup models
    \brief Keeps track of buffers.

    IrcBufferModel automatically keeps track of channel and query buffers
    and manages IrcBuffer instances for them. It will notify via signals
    when channel and query buffers are added and/or removed. IrcBufferModel
    can be used directly as a data model for Qt's item views - both in C++
    and QML.

    \code
    IrcConnection* connection = new IrcConnection(this);
    IrcBufferModel* model = new IrcBufferModel(connection);
    connect(model, SIGNAL(added(IrcBuffer*)), this, SLOT(onBufferAdded(IrcBuffer*)));
    connect(model, SIGNAL(removed(IrcBuffer*)), this, SLOT(onBufferRemoved(IrcBuffer*)));
    listView->setModel(model);
    \endcode
 */

/*!
    \fn void IrcBufferModel::added(IrcBuffer* buffer)

    This signal is emitted when a \a buffer is added to the list of buffers.
 */

/*!
    \fn void IrcBufferModel::removed(IrcBuffer* buffer)

    This signal is emitted when a \a buffer is removed from the list of buffers.
 */

/*!
    \fn void IrcBufferModel::aboutToBeAdded(IrcBuffer* buffer)

    This signal is emitted just before a \a buffer is added to the list of buffers.
 */

/*!
    \fn void IrcBufferModel::aboutToBeRemoved(IrcBuffer* buffer)

    This signal is emitted just before a \a buffer is removed from the list of buffers.
 */

/*!
    \fn void IrcBufferModel::messageIgnored(IrcMessage* message)

    This signal is emitted when a message was ignored.

    IrcBufferModel handles only buffer specific messages and delivers
    them to the appropriate IrcBuffer instances. When applications decide
    to handle IrcBuffer::messageReceived(), this signal makes it easy to
    implement handling for the rest, non-buffer specific messages.

    \sa IrcConnection::messageReceived(), IrcBuffer::messageReceived()
 */

#ifndef IRC_DOXYGEN
class IrcBufferLessThan
{
public:
    IrcBufferLessThan(IrcBufferModel* model, Irc::SortMethod method) : model(model), method(method) { }
    bool operator()(IrcBuffer* b1, IrcBuffer* b2) const { return model->lessThan(b1, b2, method); }
private:
    IrcBufferModel* model;
    Irc::SortMethod method;
};

class IrcBufferGreaterThan
{
public:
    IrcBufferGreaterThan(IrcBufferModel* model, Irc::SortMethod method) : model(model), method(method) { }
    bool operator()(IrcBuffer* b1, IrcBuffer* b2) const { return model->lessThan(b2, b1, method); }
private:
    IrcBufferModel* model;
    Irc::SortMethod method;
};

static QHash<int, QByteArray> irc_buffer_model_roles()
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[Irc::BufferRole] = "buffer";
    roles[Irc::ChannelRole] = "channel";
    roles[Irc::NameRole] = "name";
    roles[Irc::PrefixRole] = "prefix";
    roles[Irc::TitleRole] = "title";
    return roles;
}

IrcBufferModelPrivate::IrcBufferModelPrivate()
{
}

bool IrcBufferModelPrivate::messageFilter(IrcMessage* msg)
{
    Q_Q(IrcBufferModel);
    if (msg->type() == IrcMessage::Join && msg->isOwn())
        createBuffer(static_cast<IrcJoinMessage*>(msg)->channel());

    bool processed = false;
    switch (msg->type()) {
        case IrcMessage::Away:
        case IrcMessage::Nick:
        case IrcMessage::Quit:
            foreach (IrcBuffer* buffer, bufferList) {
                if (buffer->isActive())
                    IrcBufferPrivate::get(buffer)->processMessage(msg);
            }
            if (msg->type() != IrcMessage::Away || !msg->isOwn())
                processed = true;
            break;

        case IrcMessage::Join:
        case IrcMessage::Part:
        case IrcMessage::Kick:
        case IrcMessage::Names:
        case IrcMessage::Topic:
            processed = processMessage(msg->property("channel").toString(), msg);
            break;

        case IrcMessage::WhoReply:
            processed = processMessage(static_cast<IrcWhoReplyMessage*>(msg)->mask(), msg);
            break;

        case IrcMessage::Private:
            if (IrcPrivateMessage* pm = static_cast<IrcPrivateMessage*>(msg))
                processed = !pm->isRequest() && processMessage(pm->isPrivate() ? pm->nick() : pm->target(), pm, true);
            break;

        case IrcMessage::Notice:
            if (IrcNoticeMessage* no = static_cast<IrcNoticeMessage*>(msg))
                processed = !no->isReply() && processMessage(no->isPrivate() ? no->nick() : no->target(), no, no->host() != QLatin1String("services."));
            break;

        case IrcMessage::Mode:
            processed = processMessage(static_cast<IrcModeMessage*>(msg)->target(), msg);
            break;

        case IrcMessage::Numeric:
            // TODO: any other special cases besides RPL_NAMREPLY?
            if (static_cast<IrcNumericMessage*>(msg)->code() == Irc::RPL_NAMREPLY) {
                const int count = msg->parameters().count();
                const QString channel = msg->parameters().value(count - 2);
                processed = processMessage(channel, msg);
            } else if (static_cast<IrcNumericMessage*>(msg)->code() == Irc::RPL_MONONLINE ||
                       static_cast<IrcNumericMessage*>(msg)->code() == Irc::RPL_MONOFFLINE) {
                msg->setFlag(IrcMessage::Implicit);
                foreach (const QString& target, msg->parameters().value(1).split(QLatin1String(",")))
                    processed |= processMessage(Irc::nickFromPrefix(target), msg);
            } else {
                processed = processMessage(msg->parameters().value(1), msg);
            }
            break;

        default:
            break;
    }

    if (!processed)
        emit q->messageIgnored(msg);

    if (!msg->testFlag(IrcMessage::Playback)) {
        if (msg->type() == IrcMessage::Part && msg->isOwn()) {
            destroyBuffer(static_cast<IrcPartMessage*>(msg)->channel());
        } else if (msg->type() == IrcMessage::Kick) {
            const IrcKickMessage* kickMsg = static_cast<IrcKickMessage*>(msg);
            if (!kickMsg->user().compare(msg->connection()->nickName(), Qt::CaseInsensitive))
                destroyBuffer(kickMsg->channel());
        }
    }
    return false;
}

bool IrcBufferModelPrivate::commandFilter(IrcCommand* cmd)
{
    if (cmd->type() == IrcCommand::Join) {
        const QString channel = cmd->parameters().value(0).toLower();
        const QString key = cmd->parameters().value(1);
        if (!key.isEmpty())
            keys.insert(channel, key);
        else
            keys.remove(channel);
    }
    return false;
}

IrcBuffer* IrcBufferModelPrivate::createBufferHelper(const QString& title)
{
    Q_Q(IrcBufferModel);
    IrcBuffer* buffer = nullptr;
    const QMetaObject* metaObject = q->metaObject();
    int idx = metaObject->indexOfMethod("createBuffer(QVariant)");
    if (idx != -1) {
        // QML: QVariant createBuffer(QVariant)
        QVariant ret;
        QMetaMethod method = metaObject->method(idx);
        method.invoke(q, Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, title));
        buffer = ret.value<IrcBuffer*>();
    } else {
        // C++: IrcBuffer* createBuffer(QString)
        idx = metaObject->indexOfMethod("createBuffer(QString)");
        QMetaMethod method = metaObject->method(idx);
        method.invoke(q, Q_RETURN_ARG(IrcBuffer*, buffer), Q_ARG(QString, title));
    }
    return buffer;
}

IrcChannel* IrcBufferModelPrivate::createChannelHelper(const QString& title)
{
    Q_Q(IrcBufferModel);
    IrcChannel* channel = nullptr;
    const QMetaObject* metaObject = q->metaObject();
    int idx = metaObject->indexOfMethod("createChannel(QVariant)");
    if (idx != -1) {
        // QML: QVariant createChannel(QVariant)
        QVariant ret;
        QMetaMethod method = metaObject->method(idx);
        method.invoke(q, Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, title));
        channel = ret.value<IrcChannel*>();
    } else {
        // C++: IrcChannel* createChannel(QString)
        idx = metaObject->indexOfMethod("createChannel(QString)");
        QMetaMethod method = metaObject->method(idx);
        method.invoke(q, Q_RETURN_ARG(IrcChannel*, channel), Q_ARG(QString, title));
    }
    return channel;
}

IrcBuffer* IrcBufferModelPrivate::createBuffer(const QString& title)
{
    Q_Q(IrcBufferModel);
    IrcBuffer* buffer = bufferMap.value(title.toLower());
    if (!buffer) {
        if (connection && connection->network()->isChannel(title))
            buffer = createChannelHelper(title);
        else
            buffer = createBufferHelper(title);
        if (buffer) {
            IrcBufferPrivate::get(buffer)->init(title, q);
            addBuffer(buffer);
        }
    }
    return buffer;
}

void IrcBufferModelPrivate::destroyBuffer(const QString& title, bool force)
{
    IrcBuffer* buffer = bufferMap.value(title.toLower());
    if (buffer && (force || (!persistent && !buffer->isPersistent()))) {
        removeBuffer(buffer);
        buffer->deleteLater();
    }
}

void IrcBufferModelPrivate::addBuffer(IrcBuffer* buffer, bool notify)
{
    insertBuffer(-1, buffer, notify);
}

void IrcBufferModelPrivate::insertBuffer(int index, IrcBuffer* buffer, bool notify)
{
    Q_Q(IrcBufferModel);
    if (buffer && !bufferList.contains(buffer)) {
        restoreBuffer(buffer);
        const QString title = buffer->title();
        const QString lower = title.toLower();
        if (bufferMap.contains(lower)) {
            qWarning() << "IrcBufferModel: ignored duplicate buffer" << title;
            return;
        }
        IrcBufferPrivate::get(buffer)->setModel(q);
        const bool isChannel = buffer->isChannel();
        if (sortMethod != Irc::SortByHand) {
            QList<IrcBuffer*>::iterator it;
            if (sortOrder == Qt::AscendingOrder)
                it = std::upper_bound(bufferList.begin(), bufferList.end(), buffer, IrcBufferLessThan(q, sortMethod));
            else
                it = std::upper_bound(bufferList.begin(), bufferList.end(), buffer, IrcBufferGreaterThan(q, sortMethod));
            index = it - bufferList.begin();
        } else if (index == -1) {
            index = bufferList.count();
        }
        if (notify)
            emit q->aboutToBeAdded(buffer);
        q->beginInsertRows(QModelIndex(), index, index);
        bufferList.insert(index, buffer);
        bufferMap.insert(lower, buffer);
        if (isChannel) {
            channels += title;
            IrcChannel* channel = buffer->toChannel();
            if (keys.contains(lower) && channel->key().isEmpty())
                IrcChannelPrivate::get(channel)->setKey(keys.take(lower));
        }
        q->connect(buffer, SIGNAL(destroyed(IrcBuffer*)), SLOT(_irc_bufferDestroyed(IrcBuffer*)));
        q->endInsertRows();
        if (notify) {
            emit q->added(buffer);
            if (isChannel)
                emit q->channelsChanged(channels);
            emit q->buffersChanged(bufferList);
            emit q->countChanged(bufferList.count());
            if (bufferList.count() == 1)
                emit q->emptyChanged(false);
        }
        if (monitorEnabled && IrcBufferPrivate::get(buffer)->isMonitorable()) {
            connection->sendCommand(IrcCommand::createMonitor(QStringLiteral("+"), buffer->title()));
            if (!monitorPending) {
                monitorPending = true;
                QTimer::singleShot(1000, q, SLOT(_irc_monitorStatus()));
            }
        }
    }
}

void IrcBufferModelPrivate::removeBuffer(IrcBuffer* buffer, bool notify)
{
    Q_Q(IrcBufferModel);
    int idx = bufferList.indexOf(buffer);
    if (idx != -1) {
        const QString title = buffer->title();
        const QString lower = title.toLower();
        const bool isChannel = buffer->isChannel();
        if (notify)
            emit q->aboutToBeRemoved(buffer);
        q->beginRemoveRows(QModelIndex(), idx, idx);
        bufferList.removeAt(idx);
        bufferMap.remove(lower);
        bufferStates.remove(lower);
        if (isChannel)
            channels.removeOne(title);
        q->endRemoveRows();
        if (notify) {
            emit q->removed(buffer);
            if (isChannel)
                emit q->channelsChanged(channels);
            emit q->buffersChanged(bufferList);
            emit q->countChanged(bufferList.count());
            if (bufferList.isEmpty())
                emit q->emptyChanged(true);
        }
        if (monitorEnabled && IrcBufferPrivate::get(buffer)->isMonitorable())
            connection->sendCommand(IrcCommand::createMonitor(QStringLiteral("-"), title));
    }
}

bool IrcBufferModelPrivate::renameBuffer(const QString& from, const QString& to)
{
    Q_Q(IrcBufferModel);
    const QString fromLower = from.toLower();
    const QString toLower = to.toLower();
    if (bufferMap.contains(toLower))
        destroyBuffer(toLower, true);
    if (bufferMap.contains(fromLower)) {
        IrcBuffer* buffer = bufferMap.take(fromLower);
        bufferMap.insert(toLower, buffer);

        const int idx = bufferList.indexOf(buffer);
        QModelIndex index = q->index(idx);
        emit q->dataChanged(index, index);

        if (sortMethod != Irc::SortByHand) {
            QList<IrcBuffer*> buffers = bufferList;
            const bool notify = false;
            removeBuffer(buffer, notify);
            insertBuffer(-1, buffer, notify);
            if (buffers != bufferList)
                emit q->buffersChanged(bufferList);
        }
        return true;
    }
    return false;
}

void IrcBufferModelPrivate::promoteBuffer(IrcBuffer* buffer)
{
    Q_Q(IrcBufferModel);
    if (sortMethod == Irc::SortByActivity) {
        const bool notify = false;
        removeBuffer(buffer, notify);
        insertBuffer(0, buffer, notify);
        emit q->buffersChanged(bufferList);
    }
}

void IrcBufferModelPrivate::restoreBuffer(IrcBuffer* buffer)
{
    const QVariantMap& b = bufferStates.value(buffer->title().toLower()).toMap();
    if (!b.isEmpty()) {
        buffer->setSticky(b.value(QStringLiteral("sticky")).toBool());
        buffer->setPersistent(b.value(QStringLiteral("persistent")).toBool());
        buffer->setUserData(b.value(QStringLiteral("userData")).toMap());
        IrcChannel* channel = buffer->toChannel();
        if (channel && !channel->isActive()) {
            IrcChannelPrivate* p = IrcChannelPrivate::get(channel);
            const QStringList modes = b.value(QStringLiteral("modes")).toStringList();
            const QStringList args = b.value(QStringLiteral("args")).toStringList();
            for (int i = 0; i < modes.count(); ++i)
                p->modes.insert(modes.at(i), args.value(i));
            p->enabled = b.value(QStringLiteral("enabled"), true).toBool();
        }
    }
}

QVariantMap IrcBufferModelPrivate::saveBuffer(IrcBuffer* buffer) const
{
    QVariantMap b;
    b.insert(QStringLiteral("title"), buffer->title());
    b.insert(QStringLiteral("name"), buffer->name());
    b.insert(QStringLiteral("prefix"), buffer->prefix());
    if (IrcChannel* channel = buffer->toChannel()) {
        IrcChannelPrivate* p = IrcChannelPrivate::get(channel);
        b.insert(QStringLiteral("modes"), QStringList(p->modes.keys()));
        b.insert(QStringLiteral("args"), QStringList(p->modes.values()));
        b.insert(QStringLiteral("topic"), channel->topic());
        b.insert(QStringLiteral("enabled"), p->enabled);
    }
    b.insert(QStringLiteral("channel"), buffer->isChannel());
    b.insert(QStringLiteral("sticky"), buffer->isSticky());
    b.insert(QStringLiteral("persistent"), buffer->isPersistent());
    b.insert(QStringLiteral("userData"), buffer->userData());
    return b;
}

bool IrcBufferModelPrivate::processMessage(const QString& title, IrcMessage* message, bool create)
{
    IrcBuffer* buffer = bufferMap.value(title.toLower());
    if (!buffer && create && title != QLatin1String("*"))
        buffer = createBuffer(title);
    if (buffer)
        return IrcBufferPrivate::get(buffer)->processMessage(message);
    return false;
}

void IrcBufferModelPrivate::_irc_connected()
{
    foreach (IrcBuffer* buffer, bufferList)
        IrcBufferPrivate::get(buffer)->connected();
}

void IrcBufferModelPrivate::_irc_initialized()
{
    Q_Q(IrcBufferModel);
    if (joinDelay >= 0)
        QTimer::singleShot(joinDelay * 1000, q, SLOT(_irc_restoreBuffers()));

    bool monitored = false;
    foreach (IrcBuffer* buffer, bufferList) {
        if (monitorEnabled && IrcBufferPrivate::get(buffer)->isMonitorable()) {
            connection->sendCommand(IrcCommand::createMonitor(QStringLiteral("+"), buffer->title()));
            monitored = true;
        }
    }

    if (monitored && !monitorPending) {
        monitorPending = true;
        QTimer::singleShot(1000, q, SLOT(_irc_monitorStatus()));
    }
}

void IrcBufferModelPrivate::_irc_disconnected()
{
    foreach (IrcBuffer* buffer, bufferList)
        IrcBufferPrivate::get(buffer)->disconnected();
}

void IrcBufferModelPrivate::_irc_bufferDestroyed(IrcBuffer* buffer)
{
    removeBuffer(buffer);
}

static bool sortIrcChannels_withKeysFirst(IrcChannel *ch1, IrcChannel *ch2) {
    return ch1->key().length() > ch2->key().length();
}

void IrcBufferModelPrivate::_irc_restoreBuffers()
{
    Q_Q(IrcBufferModel);
    if (!connection || !connection->isConnected())
        return;

    bool hasActiveChannels = false;
    foreach (IrcBuffer* buffer, bufferList) {
        if (buffer->isChannel() && buffer->isActive()) {
            // this is probably a bouncer connection if there are already
            // active channels. don't restore and re-join channels that were
            // left in another client meanwhile this client was disconnected.
            hasActiveChannels = true;
        }
    }

    if (!hasActiveChannels) {
        foreach (const QVariant& v, bufferStates) {
            QVariantMap b = v.toMap();
            IrcBuffer* buffer = q->find(b.value(QStringLiteral("title")).toString());
            if (!buffer) {
                if (b.value(QStringLiteral("channel")).toBool())
                    buffer = createChannelHelper(b.value(QStringLiteral("title")).toString());
                else
                    buffer = createBufferHelper(b.value(QStringLiteral("title")).toString());
                buffer->setName(b.value(QStringLiteral("name")).toString());
                buffer->setPrefix(b.value(QStringLiteral("prefix")).toString());
                q->add(buffer);
            }
        }

        // Join multiple channels:
        //
        // * A single IRC command may be 512 bytes long, including the <CR><LF> at the end.
        //   Therefore, we must collect as many channels as many channels as possible into
        //   a single command, but without exceeding the 512 bytes.
        //   NOTES:
        //       - Previously, communi tried to join all channels in a single command,
        //         which didn't work because it exceeded the 512 bytes
        //       - Then, it joined 3 channels at a time
        //
        // * We should also group channels with keys separately from channels without keys,
        //   because the JOIN command doesn't work when channels without keys preceed the
        //   channels with keys.
        //   Works:
        //       JOIN #ch1,#ch2             --- neither #ch1 nor #ch2 have keys
        //       JOIN #ch1,#ch2 key1,key2   --- both #ch1 and #ch2 have keys
        //       JOIN #ch1,#ch2 key1        --- #ch1 has key, #ch2 doesn't
        //   Doesn't work:
        //       JOIN #ch1,#ch2 ,key2       --- #ch1 doesn't have a key, #ch2 does
        //                                      (NOTE: this is what communi used to do)
        //

        // Get channels from buffers
        QList<IrcChannel*> filteredChannels;

        foreach (IrcBuffer* buf, bufferList) {
            IrcChannel *channel = buf->toChannel();
            if (channel && !channel->isActive() && IrcChannelPrivate::get(channel)->enabled) {
                filteredChannels.append(channel);
            }
        }

        // Sort channels with keys first
        std::sort(filteredChannels.begin(), filteredChannels.end(), sortIrcChannels_withKeysFirst);

        if (filteredChannels.size()) {

            // Length of "JOIN  \r\n"
            const int joinCommandMinLength = 8;
            const int maxIrcCommandBytes = 512;
            int joinCommandLength = joinCommandMinLength;

            QStringList chans, keys;
            foreach (IrcChannel *channel, filteredChannels) {
                int additonalLength = channel->title().length() + channel->key().length();
                // Command needs a comma between channels
                if (chans.length())
                    additonalLength++;
                // Command needs a comma between keys
                if (keys.length() && channel->key().length())
                    additonalLength++;

                if ((joinCommandLength + additonalLength) > maxIrcCommandBytes) {
                    // If the command size would exceed the maximum, send a command with the channels collected so far
                    connection->sendCommand(IrcCommand::createJoin(chans, keys));

                    chans.clear();
                    keys.clear();
                    joinCommandLength = joinCommandMinLength;
                }

                // Add channel to list
                chans += channel->title();

                // Only add key to list if there is a key,
                // otherwise not needed, because channels with keys are sorted before channels without keys
                if (channel->key().length())
                    keys += channel->key();

                joinCommandLength = additonalLength;
            }

            if (!chans.isEmpty()) {
                connection->sendCommand(IrcCommand::createJoin(chans, keys));
            }

        }
    }
}

void IrcBufferModelPrivate::_irc_monitorStatus()
{
    if (monitorEnabled && connection)
        connection->sendCommand(IrcCommand::createMonitor(QStringLiteral("S")));
    monitorPending = false;
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new model with \a parent.

    \note If \a parent is an instance of IrcConnection, it will be
    automatically assigned to \ref IrcBufferModel::connection "connection".
 */
IrcBufferModel::IrcBufferModel(QObject* parent)
    : QAbstractListModel(parent), d_ptr(new IrcBufferModelPrivate)
{
    Q_D(IrcBufferModel);
    d->q_ptr = this;
    setBufferPrototype(new IrcBuffer(this));
    setChannelPrototype(new IrcChannel(this));
    setConnection(qobject_cast<IrcConnection*>(parent));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    setRoleNames(irc_buffer_model_roles());
#endif
}

/*!
    Destructs the model.
 */
IrcBufferModel::~IrcBufferModel()
{
    Q_D(IrcBufferModel);
    foreach (IrcBuffer* buffer, d->bufferList) {
        buffer->disconnect(this);
        delete buffer;
    }
    d->bufferList.clear();
    d->bufferMap.clear();
    d->channels.clear();
    emit destroyed(this);
}

/*!
    This property holds the connection.

    \par Access functions:
    \li \ref IrcConnection* <b>connection</b>() const
    \li void <b>setConnection</b>(\ref IrcConnection* connection)

    \warning Changing the connection on the fly is not supported.
 */
IrcConnection* IrcBufferModel::connection() const
{
    Q_D(const IrcBufferModel);
    return d->connection;
}

void IrcBufferModel::setConnection(IrcConnection* connection)
{
    Q_D(IrcBufferModel);
    if (d->connection != connection) {
        if (d->connection) {
            qCritical("IrcBufferModel::setConnection(): changing the connection on the fly is not supported.");
            return;
        }
        d->connection = connection;
        d->connection->installMessageFilter(d);
        d->connection->installCommandFilter(d);
        connect(d->connection, SIGNAL(connected()), this, SLOT(_irc_connected()));
        connect(d->connection, SIGNAL(disconnected()), this, SLOT(_irc_disconnected()));
        connect(d->connection->network(), SIGNAL(initialized()), this, SLOT(_irc_initialized()));
        emit connectionChanged(connection);
        emit networkChanged(network());
    }
}

/*!
    This property holds the network.

    \par Access functions:
    \li \ref IrcNetwork* <b>network</b>() const
 */
IrcNetwork* IrcBufferModel::network() const
{
    Q_D(const IrcBufferModel);
    return d->connection ? d->connection->network() : nullptr;
}

/*!
    This property holds the number of buffers.

    \par Access function:
    \li int <b>count</b>() const

    \par Notifier signal:
    \li void <b>countChanged</b>(int count)
 */
int IrcBufferModel::count() const
{
    return rowCount();
}

/*!
    \since 3.1
    \property bool IrcBufferModel::empty

    This property holds the whether the model is empty.

    \par Access function:
    \li bool <b>isEmpty</b>() const

    \par Notifier signal:
    \li void <b>emptyChanged</b>(bool empty)
 */
bool IrcBufferModel::isEmpty() const
{
    Q_D(const IrcBufferModel);
    return d->bufferList.isEmpty();
}

/*!
    This property holds the list of channel names.

    \par Access function:
    \li QStringList <b>channels</b>() const

    \par Notifier signal:
    \li void <b>channelsChanged</b>(const QStringList& channels)
 */
QStringList IrcBufferModel::channels() const
{
    Q_D(const IrcBufferModel);
    return d->channels;
}

/*!
    This property holds the list of buffers.

    \par Access function:
    \li QList<\ref IrcBuffer*> <b>buffers</b>() const

    \par Notifier signal:
    \li void <b>buffersChanged</b>(const QList<\ref IrcBuffer*>& buffers)
 */
QList<IrcBuffer*> IrcBufferModel::buffers() const
{
    Q_D(const IrcBufferModel);
    return d->bufferList;
}

/*!
    Returns the buffer object at \a index.
 */
IrcBuffer* IrcBufferModel::get(int index) const
{
    Q_D(const IrcBufferModel);
    return d->bufferList.value(index);
}

/*!
    Returns the buffer object for \a title or \c 0 if not found.
 */
IrcBuffer* IrcBufferModel::find(const QString& title) const
{
    Q_D(const IrcBufferModel);
    return d->bufferMap.value(title.toLower());
}

/*!
    Returns \c true if the model contains \a title.
 */
bool IrcBufferModel::contains(const QString& title) const
{
    Q_D(const IrcBufferModel);
    return d->bufferMap.contains(title.toLower());
}

/*!
    Returns the index of the specified \a buffer,
    or \c -1 if the model does not contain the \a buffer.
 */
int IrcBufferModel::indexOf(IrcBuffer* buffer) const
{
    Q_D(const IrcBufferModel);
    return d->bufferList.indexOf(buffer);
}

/*!
    Adds a buffer with \a title to the model and returns it.
 */
IrcBuffer* IrcBufferModel::add(const QString& title)
{
    Q_D(IrcBufferModel);
    return d->createBuffer(title);
}

/*!
    Adds the \a buffer to the model.
 */
void IrcBufferModel::add(IrcBuffer* buffer)
{
    Q_D(IrcBufferModel);
    d->addBuffer(buffer);
}

/*!
    Removes and destroys a buffer with \a title from the model.
 */
void IrcBufferModel::remove(const QString& title)
{
    Q_D(IrcBufferModel);
    d->destroyBuffer(title, true);
}

/*!
    Removes and destroys a \a buffer from the model.
 */
void IrcBufferModel::remove(IrcBuffer* buffer)
{
    delete buffer;
}

/*!
    This property holds the display role.

    The specified data role is returned for Qt::DisplayRole.

    The default value is \ref Irc::TitleRole.

    \par Access functions:
    \li \ref Irc::DataRole <b>displayRole</b>() const
    \li void <b>setDisplayRole</b>(\ref Irc::DataRole role)
 */
Irc::DataRole IrcBufferModel::displayRole() const
{
    Q_D(const IrcBufferModel);
    return d->role;
}

void IrcBufferModel::setDisplayRole(Irc::DataRole role)
{
    Q_D(IrcBufferModel);
    d->role = role;
}

/*!
    \since 3.1

    \property bool IrcBufferModel::persistent
    This property holds whether the model is persistent.

    The default value is \c false.

    A persistent model does not remove and destruct channel buffers
    automatically when leaving the corresponding channels. In order
    to remove buffers from a persistent model, either call
    IrcBufferModel::remove() or delete the buffer.

    \par Access functions:
    \li bool <b>isPersistent</b>() const
    \li void <b>setPersistent</b>(bool persistent)

    \par Notifier signal:
    \li void <b>persistentChanged</b>(bool persistent)
 */
bool IrcBufferModel::isPersistent() const
{
    Q_D(const IrcBufferModel);
    return d->persistent;
}

void IrcBufferModel::setPersistent(bool persistent)
{
    Q_D(IrcBufferModel);
    if (d->persistent != persistent) {
        d->persistent = persistent;
        emit persistentChanged(persistent);
    }
}

/*!
    Returns the model index for \a buffer.
 */
QModelIndex IrcBufferModel::index(IrcBuffer* buffer) const
{
    Q_D(const IrcBufferModel);
    return index(d->bufferList.indexOf(buffer));
}

/*!
    Returns the buffer for model \a index.
 */
IrcBuffer* IrcBufferModel::buffer(const QModelIndex& index) const
{
    if (!hasIndex(index.row(), index.column()))
        return nullptr;

    return static_cast<IrcBuffer*>(index.internalPointer());
}

/*!
    This property holds the model sort order.

    The default value is \c Qt::AscendingOrder.

    \par Access functions:
    \li Qt::SortOrder <b>sortOrder</b>() const
    \li void <b>setSortOrder</b>(Qt::SortOrder order)

    \sa sort(), lessThan()
 */
Qt::SortOrder IrcBufferModel::sortOrder() const
{
    Q_D(const IrcBufferModel);
    return d->sortOrder;
}

void IrcBufferModel::setSortOrder(Qt::SortOrder order)
{
    Q_D(IrcBufferModel);
    if (d->sortOrder != order) {
        d->sortOrder = order;
        if (d->sortMethod != Irc::SortByHand && !d->bufferList.isEmpty())
            sort(d->sortMethod, d->sortOrder);
    }
}

/*!
    This property holds the model sort method.

    The default value is \c Irc::SortByHand.

    Method              | Description                                                                      | Example
    --------------------|----------------------------------------------------------------------------------|-------------------------------------------------
    Irc::SortByHand     | Buffers are not sorted automatically, but only by calling sort().                | -
    Irc::SortByName     | Buffers are sorted alphabetically, ignoring any channel prefix.                  | "bot", "#communi", "#libera", "jpnurmi", "#qt"
    Irc::SortByTitle    | Buffers are sorted alphabetically, and channels before queries.                  | "#communi", "#libera", "#qt", "bot", "jpnurmi"
    Irc::SortByActivity | Buffers are sorted based on their messaging activity, last active buffers first. | -

    \note Irc::SortByActivity support was added in version \b 3.4.

    \par Access functions:
    \li Irc::SortMethod <b>sortMethod</b>() const
    \li void <b>setSortMethod</b>(Irc::SortMethod method)

    \sa sort(), lessThan()
 */
Irc::SortMethod IrcBufferModel::sortMethod() const
{
    Q_D(const IrcBufferModel);
    return d->sortMethod;
}

void IrcBufferModel::setSortMethod(Irc::SortMethod method)
{
    Q_D(IrcBufferModel);
    if (d->sortMethod != method) {
        d->sortMethod = method;
        if (d->sortMethod != Irc::SortByHand && !d->bufferList.isEmpty())
            sort(d->sortMethod, d->sortOrder);
    }
}

/*!
    Clears the model.

    All buffers except \ref IrcBuffer::persistent "persistent" buffers are removed and destroyed.

    In order to remove a persistent buffer, either explicitly call remove() or delete the buffer.
 */
void IrcBufferModel::clear()
{
    Q_D(IrcBufferModel);
    if (!d->bufferList.isEmpty()) {
        bool bufferRemoved = false;
        bool channelRemoved = false;
        foreach (IrcBuffer* buffer, d->bufferList) {
            if (!buffer->isPersistent()) {
                if (!bufferRemoved) {
                    beginResetModel();
                    bufferRemoved = true;
                }
                channelRemoved |= buffer->isChannel();
                buffer->disconnect(this);
                d->bufferList.removeOne(buffer);
                d->channels.removeOne(buffer->title());
                d->bufferMap.remove(buffer->title().toLower());
                delete buffer;
            }
        }
        if (bufferRemoved) {
            endResetModel();
            if (channelRemoved)
                emit channelsChanged(d->channels);
            emit buffersChanged(d->bufferList);
            emit countChanged(d->bufferList.count());
            if (d->bufferList.isEmpty())
                emit emptyChanged(true);
        }
    }
}

/*!
    Makes the model receive and handle \a message.
 */
void IrcBufferModel::receiveMessage(IrcMessage* message)
{
    Q_D(IrcBufferModel);
    d->messageFilter(message);
}

/*!
    Sorts the model using the given \a order.
 */
void IrcBufferModel::sort(int column, Qt::SortOrder order)
{
    Q_D(IrcBufferModel);
    if (column == 0)
        sort(d->sortMethod, order);
}

/*!
    Sorts the model using the given \a method and \a order.

    \sa lessThan()
 */
void IrcBufferModel::sort(Irc::SortMethod method, Qt::SortOrder order)
{
    Q_D(IrcBufferModel);
    if (method == Irc::SortByHand)
        return;

    emit layoutAboutToBeChanged();

    QList<IrcBuffer*> persistentBuffers;
    QModelIndexList oldPersistentIndexes = persistentIndexList();
    foreach (const QModelIndex& index, oldPersistentIndexes)
        persistentBuffers += static_cast<IrcBuffer*>(index.internalPointer());

    if (order == Qt::AscendingOrder)
        std::sort(d->bufferList.begin(), d->bufferList.end(), IrcBufferLessThan(this, method));
    else
        std::sort(d->bufferList.begin(), d->bufferList.end(), IrcBufferGreaterThan(this, method));

    QModelIndexList newPersistentIndexes;
    foreach (IrcBuffer* buffer, persistentBuffers)
        newPersistentIndexes += index(d->bufferList.indexOf(buffer));
    changePersistentIndexList(oldPersistentIndexes, newPersistentIndexes);

    emit layoutChanged();
}

/*!
    Creates a buffer object with \a title.

    IrcBufferModel will automatically call this factory method when a
    need for the buffer object occurs ie. a private message is received.

    The default implementation creates an instance of the buffer prototype.
    Reimplement this function in order to alter the default behavior.

    \sa bufferPrototype
 */
IrcBuffer* IrcBufferModel::createBuffer(const QString& title)
{
    Q_D(IrcBufferModel);
    Q_UNUSED(title);
    return d->bufferProto->clone(this);
}

/*!
    Creates a channel object with \a title.

    IrcBufferModel will automatically call this factory method when a
    need for the channel object occurs ie. a channel is being joined.

    The default implementation creates an instance of the channel prototype.
    Reimplement this function in order to alter the default behavior.

    \sa channelPrototype
 */
IrcChannel* IrcBufferModel::createChannel(const QString& title)
{
    Q_D(IrcBufferModel);
    Q_UNUSED(title);
    return qobject_cast<IrcChannel*>(d->channelProto->clone(this));
}

/*!
    Returns \c true if \a one buffer is "less than" \a another,
    otherwise returns \c false.

    The default implementation sorts according to the specified sort method.
    Reimplement this function in order to customize the sort order.

    \sa sort(), sortMethod
 */
bool IrcBufferModel::lessThan(IrcBuffer* one, IrcBuffer* another, Irc::SortMethod method) const
{
    if (one->isSticky() != another->isSticky())
        return one->isSticky();

    if (method == Irc::SortByActivity) {
        QDateTime ts1 = IrcBufferPrivate::get(one)->activity;
        QDateTime ts2 = IrcBufferPrivate::get(another)->activity;
        if (ts1.isValid() || ts2.isValid())
            return ts1.isValid() && ts1 > ts2;
    }

    if (method == Irc::SortByTitle) {
        const QStringList prefixes = one->network()->channelTypes();

        const QString p1 = one->prefix();
        const QString p2 = another->prefix();

        const int i1 = !p1.isEmpty() ? prefixes.indexOf(p1.at(0)) : -1;
        const int i2 = !p2.isEmpty() ? prefixes.indexOf(p2.at(0)) : -1;

        if (i1 >= 0 && i2 < 0)
            return true;
        if (i1 < 0 && i2 >= 0)
            return false;
        if (i1 >= 0 && i2 >= 0 && i1 != i2)
            return i1 < i2;
    }

    // Irc::SortByName
    const QString n1 = one->name();
    const QString n2 = another->name();
    return n1.compare(n2, Qt::CaseInsensitive) < 0;
}

/*!
    The following role names are provided by default:

    Role             | Name       | Type        | Example
    -----------------|------------|-------------|--------
    Qt::DisplayRole  | "display"  | 1)          | -
    Irc::BufferRole  | "buffer"   | IrcBuffer*  | &lt;object&gt;
    Irc::ChannelRole | "channel"  | IrcChannel* | &lt;object&gt;
    Irc::NameRole    | "name"     | QString     | "communi"
    Irc::PrefixRole  | "prefix"   | QString     | "#"
    Irc::TitleRole   | "title"    | QString     | "#communi"

    1) The type depends on \ref displayRole.
 */
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
QHash<int, QByteArray> IrcBufferModel::roleNames() const
{
    return irc_buffer_model_roles();
}
#endif

/*!
    Returns the number of buffers.
 */
int IrcBufferModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    Q_D(const IrcBufferModel);
    return d->bufferList.count();
}

/*!
    Returns the data for specified \a role and user referred to by by the \a index.
 */
QVariant IrcBufferModel::data(const QModelIndex& index, int role) const
{
    Q_D(const IrcBufferModel);
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return QVariant();

    IrcBuffer* buffer = static_cast<IrcBuffer*>(index.internalPointer());
    Q_ASSERT(buffer);

    switch (role) {
    case Qt::DisplayRole:
        return data(index, d->role);
    case Irc::BufferRole:
        return QVariant::fromValue(buffer);
    case Irc::ChannelRole:
        return QVariant::fromValue(buffer->toChannel());
    case Irc::NameRole:
        return buffer->name();
    case Irc::PrefixRole:
        return buffer->prefix();
    case Irc::TitleRole:
        return buffer->title();
    }

    return QVariant();
}

/*!
    Returns the index of the item in the model specified by the given \a row, \a column and \a parent index.
 */
QModelIndex IrcBufferModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_D(const IrcBufferModel);
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, d->bufferList.at(row));
}

/*!
    This property holds the buffer prototype.

    The prototype is used by the default implementation of createBuffer().

    \note A custom buffer prototype must reimplement \ref IrcBuffer::clone().

    \par Access functions:
    \li \ref IrcBuffer* <b>bufferPrototype</b>() const
    \li void <b>setBufferPrototype</b>(\ref IrcBuffer* prototype)
 */
IrcBuffer* IrcBufferModel::bufferPrototype() const
{
    Q_D(const IrcBufferModel);
    return d->bufferProto;
}

void IrcBufferModel::setBufferPrototype(IrcBuffer* prototype)
{
    Q_D(IrcBufferModel);
    if (d->bufferProto != prototype) {
        if (d->bufferProto && d->bufferProto->parent() == this)
            delete d->bufferProto;
        d->bufferProto = prototype ? prototype : new IrcBuffer(this);
        emit bufferPrototypeChanged(d->bufferProto);
    }
}

/*!
    This property holds the channel prototype.

    The prototype is used by the default implementation of createChannel().

    \note A custom channel prototype must reimplement \ref IrcChannel::clone().

    \par Access functions:
    \li \ref IrcChannel* <b>channelPrototype</b>() const
    \li void <b>setChannelPrototype</b>(\ref IrcChannel* prototype)
 */
IrcChannel* IrcBufferModel::channelPrototype() const
{
    Q_D(const IrcBufferModel);
    return d->channelProto;
}

void IrcBufferModel::setChannelPrototype(IrcChannel* prototype)
{
    Q_D(IrcBufferModel);
    if (d->channelProto != prototype) {
        if (d->channelProto && d->channelProto->parent() == this)
            delete d->channelProto;
        d->channelProto = prototype ? prototype : new IrcChannel(this);
        emit channelPrototypeChanged(d->channelProto);
    }
}

/*!
    \since 3.3

    This property holds the join delay in seconds.

    The default value is \c 0 - channels are joined immediately
    after getting connected. A negative value disables automatic
    joining of channels.

    \par Access function:
    \li int <b>joinDelay</b>() const
    \li void <b>setJoinDelay</b>(int delay)

    \par Notifier signal:
    \li void <b>joinDelayChanged</b>(int delay)
 */
int IrcBufferModel::joinDelay() const
{
    Q_D(const IrcBufferModel);
    return d->joinDelay;
}

void IrcBufferModel::setJoinDelay(int delay)
{
    Q_D(IrcBufferModel);
    if (d->joinDelay != delay) {
        d->joinDelay = delay;
        emit joinDelayChanged(delay);
    }
}

/*!
    \since 3.4
    \property bool IrcBufferModel::monitorEnabled

    This property holds whether automatic monitor is enabled.

    The default value is \c false.

    \par Access function:
    \li bool <b>isMonitorEnabled</b>() const
    \li void <b>setMonitorEnabled</b>(bool enabled)

    \par Notifier signal:
    \li void <b>monitorEnabledChanged</b>(bool enabled)

    \sa \ref ircv3
 */
bool IrcBufferModel::isMonitorEnabled() const
{
    Q_D(const IrcBufferModel);
    return d->monitorEnabled;
}

void IrcBufferModel::setMonitorEnabled(bool enabled)
{
    Q_D(IrcBufferModel);
    if (d->monitorEnabled != enabled) {
        d->monitorEnabled = enabled;
        emit monitorEnabledChanged(enabled);
    }
}

/*!
    \since 3.1

    Saves the state of the model. The \a version number is stored as part of the state data.

    To restore the saved state, pass the return value and \a version number to restoreState().
 */
QByteArray IrcBufferModel::saveState(int version) const
{
    Q_D(const IrcBufferModel);
    QVariantMap args;
    args.insert(QStringLiteral("version"), version);

    QVariantMap states = d->bufferStates;
    foreach (IrcBuffer* buffer, d->bufferList)
        states.insert(buffer->title(), d->saveBuffer(buffer));

    QVariantList buffers;
    foreach (const QVariant& b, states)
        buffers += b;
    args.insert(QStringLiteral("buffers"), buffers);

    QByteArray state;
    QDataStream out(&state, QIODevice::WriteOnly);
    out << args;
    return state;
}

/*!
    \since 3.1

    Restores the \a state of the model. The \a version number is compared with that stored in \a state.
    If they do not match, the model state is left unchanged, and this function returns \c false; otherwise,
    the state is restored, and \c true is returned.

    \sa saveState()
 */
bool IrcBufferModel::restoreState(const QByteArray& state, int version)
{
    Q_D(IrcBufferModel);
    QVariantMap args;
    QDataStream in(state);
    in >> args;
    if (in.status() != QDataStream::Ok || args.value(QStringLiteral("version"), -1).toInt() != version)
        return false;

    const QVariantList buffers = args.value(QStringLiteral("buffers")).toList();
    foreach (const QVariant& v, buffers) {
        const QVariantMap b = v.toMap();
        d->bufferStates.insert(b.value(QStringLiteral("title")).toString(), b);
    }

    if (d->joinDelay >= 0 && d->connection && d->connection->isConnected())
        QTimer::singleShot(d->joinDelay * 1000, this, SLOT(_irc_restoreBuffers()));

    return true;
}

#include "moc_ircbuffermodel.cpp"
#include "moc_ircbuffermodel_p.cpp"

IRC_END_NAMESPACE
