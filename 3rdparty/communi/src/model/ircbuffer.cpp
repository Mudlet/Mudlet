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

#include "ircbuffer.h"
#include "ircbuffer_p.h"
#include "ircbuffermodel.h"
#include "ircbuffermodel_p.h"
#include "ircconnection.h"
#include "ircnetwork.h"
#include "ircchannel.h"

IRC_BEGIN_NAMESPACE

/*!
    \file ircbuffer.h
    \brief \#include &lt;IrcBuffer&gt;
 */

/*!
    \class IrcBuffer ircbuffer.h <IrcBuffer>
    \ingroup models
    \brief Keeps track of buffer status.

    \sa IrcBufferModel
*/

/*!
    \fn void IrcBuffer::messageReceived(IrcMessage* message)

    This signal is emitted when a buffer specific message is received.

    The message may one of the following types:
    - IrcMessage::Away
    - IrcMessage::Join
    - IrcMessage::Kick
    - IrcMessage::Mode
    - IrcMessage::Names
    - IrcMessage::Nick
    - IrcMessage::Notice
    - IrcMessage::Numeric
    - IrcMessage::Part
    - IrcMessage::Private
    - IrcMessage::Quit
    - IrcMessage::Topic

    \sa IrcConnection::messageReceived(), IrcBufferModel::messageIgnored()
 */

#ifndef IRC_DOXYGEN
IrcBufferPrivate::IrcBufferPrivate()
    : q_ptr(0), model(0), persistent(false), sticky(false), monitorStatus(MonitorUnknown)
{
    qRegisterMetaType<IrcBuffer*>();
    qRegisterMetaType<QList<IrcBuffer*> >();
}

IrcBufferPrivate::~IrcBufferPrivate()
{
}

void IrcBufferPrivate::init(const QString& title, IrcBufferModel* m)
{
    name = title;
    setModel(m);
}

void IrcBufferPrivate::connected()
{
    Q_Q(IrcBuffer);
    emit q->activeChanged(q->isActive());
}

void IrcBufferPrivate::disconnected()
{
    Q_Q(IrcBuffer);
    emit q->activeChanged(q->isActive());
}

void IrcBufferPrivate::setName(const QString& value)
{
    Q_Q(IrcBuffer);
    if (name != value) {
        const QString oldTitle = q->title();
        name = value;
        emit q->nameChanged(name);
        emit q->titleChanged(q->title());
        if (model)
            IrcBufferModelPrivate::get(model)->renameBuffer(oldTitle, q->title());
    }
}

void IrcBufferPrivate::setPrefix(const QString& value)
{
    Q_Q(IrcBuffer);
    if (prefix != value) {
        const QString oldTitle = q->title();
        prefix = value;
        emit q->prefixChanged(prefix);
        emit q->titleChanged(q->title());
        if (model)
            IrcBufferModelPrivate::get(model)->renameBuffer(oldTitle, q->title());
    }
}

void IrcBufferPrivate::setModel(IrcBufferModel* value)
{
    model = value;
}

void IrcBufferPrivate::setMonitorStatus(MonitorStatus status)
{
    Q_Q(IrcBuffer);
    if (monitorStatus != status) {
        bool wasActive = q->isActive();
        monitorStatus = status;
        bool isActive = q->isActive();
        if (wasActive != isActive)
            emit q->activeChanged(isActive);
    }
}

bool IrcBufferPrivate::isMonitorable() const
{
    Q_Q(const IrcBuffer);
    IrcNetwork* n = q->network();
    IrcConnection* c = q->connection();
    if (!sticky && !name.startsWith(QLatin1String("*")) && c && c->isConnected() && n && n->numericLimit(IrcNetwork::MonitorCount) >= 0 && !n->isChannel(q->title()))
        return true;
    return false;
}

bool IrcBufferPrivate::processMessage(IrcMessage* message)
{
    Q_Q(IrcBuffer);
    bool processed = false;
    switch (message->type()) {
    case IrcMessage::Away:
        processed = processAwayMessage(static_cast<IrcAwayMessage*>(message));
        break;
    case IrcMessage::Join:
        processed = processJoinMessage(static_cast<IrcJoinMessage*>(message));
        break;
    case IrcMessage::Kick:
        processed = processKickMessage(static_cast<IrcKickMessage*>(message));
        break;
    case IrcMessage::Mode:
        processed = processModeMessage(static_cast<IrcModeMessage*>(message));
        break;
    case IrcMessage::Names:
        processed = processNamesMessage(static_cast<IrcNamesMessage*>(message));
        break;
    case IrcMessage::Nick:
        processed = processNickMessage(static_cast<IrcNickMessage*>(message));
        break;
    case IrcMessage::Notice:
        processed = processNoticeMessage(static_cast<IrcNoticeMessage*>(message));
        break;
    case IrcMessage::Numeric:
        processed = processNumericMessage(static_cast<IrcNumericMessage*>(message));
        break;
    case IrcMessage::Part:
        processed = processPartMessage(static_cast<IrcPartMessage*>(message));
        break;
    case IrcMessage::Private:
        processed = processPrivateMessage(static_cast<IrcPrivateMessage*>(message));
        if (processed) {
            activity = message->timeStamp();
            IrcBufferModelPrivate::get(model)->promoteBuffer(q);
        }
        break;
    case IrcMessage::Quit:
        processed = processQuitMessage(static_cast<IrcQuitMessage*>(message));
        break;
    case IrcMessage::Topic:
        processed = processTopicMessage(static_cast<IrcTopicMessage*>(message));
        break;
    case IrcMessage::WhoReply:
        processed = processWhoReplyMessage(static_cast<IrcWhoReplyMessage*>(message));
        break;
    default:
        break;
    }
    if (processed)
        emit q->messageReceived(message);
    return processed;
}

bool IrcBufferPrivate::processAwayMessage(IrcAwayMessage* message)
{
    return !message->nick().compare(name, Qt::CaseInsensitive);
}

bool IrcBufferPrivate::processJoinMessage(IrcJoinMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processKickMessage(IrcKickMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processModeMessage(IrcModeMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processNamesMessage(IrcNamesMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processNickMessage(IrcNickMessage* message)
{
    if (!message->testFlag(IrcMessage::Playback) && !message->nick().compare(name, Qt::CaseInsensitive)) {
        setName(message->newNick());
        return true;
    }
    return !message->newNick().compare(name, Qt::CaseInsensitive);
}

bool IrcBufferPrivate::processNoticeMessage(IrcNoticeMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processNumericMessage(IrcNumericMessage* message)
{
    if (message->code() == Irc::RPL_MONONLINE)
        setMonitorStatus(MonitorOnline);
    else if (message->code() == Irc::RPL_MONOFFLINE)
        setMonitorStatus(MonitorOffline);
    return message->isImplicit();
}

bool IrcBufferPrivate::processPartMessage(IrcPartMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processPrivateMessage(IrcPrivateMessage* message)
{
    Q_UNUSED(message);
    return true;
}

bool IrcBufferPrivate::processQuitMessage(IrcQuitMessage* message)
{
    return !message->nick().compare(name, Qt::CaseInsensitive);
}

bool IrcBufferPrivate::processTopicMessage(IrcTopicMessage* message)
{
    Q_UNUSED(message);
    return false;
}

bool IrcBufferPrivate::processWhoReplyMessage(IrcWhoReplyMessage *message)
{
    Q_UNUSED(message);
    return false;
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new buffer object with \a parent.
 */
IrcBuffer::IrcBuffer(QObject* parent)
    : QObject(parent), d_ptr(new IrcBufferPrivate)
{
    Q_D(IrcBuffer);
    d->q_ptr = this;
}

/*!
    \internal
 */
IrcBuffer::IrcBuffer(IrcBufferPrivate& dd, QObject* parent)
    : QObject(parent), d_ptr(&dd)
{
    Q_D(IrcBuffer);
    d->q_ptr = this;
}

/*!
    Destructs the buffer object.
 */
IrcBuffer::~IrcBuffer()
{
    emit destroyed(this);
}

/*!
    This property holds the whole buffer title.

    The title consists of \ref prefix and \ref name.

    \par Access function:
    \li QString <b>title</b>() const

    \par Notifier signal:
    \li void <b>titleChanged</b>(const QString& title)
 */
QString IrcBuffer::title() const
{
    Q_D(const IrcBuffer);
    return d->prefix + d->name;
}

/*!
    This property holds the name part of the buffer \ref title.

    \par Access functions:
    \li QString <b>name</b>() const
    \li void <b>setName</b>(const QString& name) [slot]

    \par Notifier signal:
    \li void <b>nameChanged</b>(const QString& name)
 */
QString IrcBuffer::name() const
{
    Q_D(const IrcBuffer);
    return d->name;
}

void IrcBuffer::setName(const QString& name)
{
    Q_D(IrcBuffer);
    d->setName(name);
}

/*!
    This property holds the prefix part of the buffer \ref title.

    \par Access functions:
    \li QString <b>prefix</b>() const
    \li void <b>setPrefix</b>(const QString& prefix) [slot]

    \par Notifier signal:
    \li void <b>prefixChanged</b>(const QString& prefix)
 */
QString IrcBuffer::prefix() const
{
    Q_D(const IrcBuffer);
    return d->prefix;
}

void IrcBuffer::setPrefix(const QString& prefix)
{
    Q_D(IrcBuffer);
    return d->setPrefix(prefix);
}

/*!
    \property bool IrcBuffer::channel
    This property holds whether the buffer is a channel.

    \par Access function:
    \li bool <b>isChannel</b>() const

    \sa toChannel()
 */
bool IrcBuffer::isChannel() const
{
    return qobject_cast<const IrcChannel*>(this);
}

/*!
    Returns the buffer cast to a IrcChannel,
    if the class is actually a channel, \c 0 otherwise.

    \sa \ref channel "isChannel()"
*/
IrcChannel* IrcBuffer::toChannel()
{
    return qobject_cast<IrcChannel*>(this);
}

/*!
    This property holds the connection of the buffer.

    \par Access function:
    \li \ref IrcConnection* <b>connection</b>() const
 */
IrcConnection* IrcBuffer::connection() const
{
    Q_D(const IrcBuffer);
    return d->model ? d->model->connection() : 0;
}

/*!
    This property holds the network of the buffer.

    \par Access function:
    \li \ref IrcNetwork* <b>network</b>() const
 */
IrcNetwork* IrcBuffer::network() const
{
    Q_D(const IrcBuffer);
    return d->model ? d->model->network() : 0;
}

/*!
    This property holds the model of the buffer.

    \par Access function:
    \li \ref IrcBufferModel* <b>model</b>() const
 */
IrcBufferModel* IrcBuffer::model() const
{
    Q_D(const IrcBuffer);
    return d->model;
}

/*!
    \property bool IrcBuffer::active
    This property holds whether the buffer is active.

    A buffer is considered active when a %connection is established. Furthermore,
    channel buffers are only considered active when the user is on the channel.

    \par Access function:
    \li bool <b>isActive</b>() const

    \par Notifier signal:
    \li void <b>activeChanged</b>(bool active)

    \sa IrcConnection::connected
 */
bool IrcBuffer::isActive() const
{
    Q_D(const IrcBuffer);
    bool connected = false;
    if (IrcConnection* c = connection())
        connected = c->isConnected();
    bool monitor = false;
    if (d->model)
        monitor = d->model->isMonitorEnabled();
    return connected && (!monitor || !d->isMonitorable() || d->monitorStatus == IrcBufferPrivate::MonitorOnline);
}

/*!
    \property bool IrcBuffer::sticky
    This property holds whether the buffer is sticky.

    A sticky buffer stays in the beginning (Qt::AscendingOrder) or
    end (Qt::DescendingOrder) of the list of buffers in IrcBufferModel.

    The default value is \c false.

    \par Access functions:
    \li bool <b>isSticky</b>() const
    \li void <b>setSticky</b>(bool sticky)

    \par Notifier signal:
    \li void <b>stickyChanged</b>(bool sticky)
 */

bool IrcBuffer::isSticky() const
{
    Q_D(const IrcBuffer);
    return d->sticky;
}

void IrcBuffer::setSticky(bool sticky)
{
    Q_D(IrcBuffer);
    if (d->sticky != sticky) {
        d->sticky = sticky;
        emit stickyChanged(sticky);
    }
}

/*!
    \property bool IrcBuffer::persistent
    This property holds whether the buffer is persistent.

    The default value is \c false.

    A persistent buffer does not get removed and destructed
    when calling IrcBufferModel::clear(), or when when leaving
    the corresponding channel. In order to remove a persistent
    buffer, either explicitly call IrcBufferModel::remove() or
    delete the buffer.

    \par Access functions:
    \li bool <b>isPersistent</b>() const
    \li void <b>setPersistent</b>(bool persistent)

    \par Notifier signal:
    \li void <b>persistentChanged</b>(bool persistent)
 */

bool IrcBuffer::isPersistent() const
{
    Q_D(const IrcBuffer);
    return d->persistent;
}

void IrcBuffer::setPersistent(bool persistent)
{
    Q_D(IrcBuffer);
    if (d->persistent != persistent) {
        d->persistent = persistent;
        emit persistentChanged(persistent);
    }
}

/*!
    \since 3.1

    This property holds arbitrary user data.

    \par Access functions:
    \li QVariantMap <b>userData</b>() const
    \li void <b>setUserData</b>(const QVariantMap& data)

    \par Notifier signal:
    \li void <b>userDataChanged</b>(const QVariantMap& data)
 */
QVariantMap IrcBuffer::userData() const
{
    Q_D(const IrcBuffer);
    return d->userData;
}

void IrcBuffer::setUserData(const QVariantMap& data)
{
    Q_D(IrcBuffer);
    if (d->userData != data) {
        d->userData = data;
        emit userDataChanged(data);
    }
}

/*!
    Sends a \a command to the server.

    This method is provided for convenience. It is equal to:
    \code
    IrcConnection* connection = buffer->connection();
    connection->sendCommand(command);
    \endcode

    \sa IrcConnection::sendCommand()
 */
bool IrcBuffer::sendCommand(IrcCommand* command)
{
    if (IrcConnection* c = connection())
        return c->sendCommand(command);
    return false;
}

/*!
    Emits messageReceived() with \a message.

    IrcBufferModel handles only buffer specific messages and delivers them
    to the appropriate IrcBuffer instances. When applications decide to handle
    IrcBuffer::messageReceived(), IrcBufferModel::messageIgnored() makes it
    easy to implement handling for the rest, non-buffer specific messages.
    This method can be used to forward such ignored messages to the desired
    buffers (for instance the one that is currently active in the GUI).
 */
void IrcBuffer::receiveMessage(IrcMessage* message)
{
    if (message)
        emit messageReceived(message);
}

/*!
    \since 3.1

    Closes the buffer with an optional \a reason.

    The default implementation removes the buffer from its \ref model.
    Furthermore, IrcChannel parts the channel with \a reason and custom
    IrcBuffer subclasses might do some additional tasks.

    \sa IrcChannel::close()
 */
void IrcBuffer::close(const QString& reason)
{
    Q_UNUSED(reason);
    Q_D(const IrcBuffer);
    if (d->model)
        d->model->remove(this);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const IrcBuffer* buffer)
{
    if (!buffer)
        return debug << "IrcBuffer(0x0) ";
    debug.nospace() << buffer->metaObject()->className() << '(' << (void*) buffer;
    if (!buffer->objectName().isEmpty())
        debug.nospace() << ", name=" << qPrintable(buffer->objectName());
    if (!buffer->title().isEmpty())
        debug.nospace() << ", title=" << qPrintable(buffer->title());
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircbuffer.cpp"

IRC_END_NAMESPACE
