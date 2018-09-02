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

#include "irccommandqueue.h"
#include "irccommandqueue_p.h"
#include "ircconnection.h"
#include "irccommand.h"

IRC_BEGIN_NAMESPACE

static const int DEFAULT_BATCH = 3;
static const int DEFAULT_INTERVAL = 2;

/*!
    \file irccommandqueue.h
    \brief \#include &lt;IrcCommandQueue&gt;
 */

/*!
    \since 3.4
    \class IrcCommandQueue irccommandqueue.h <IrcCommandQueue>
    \ingroup util
    \brief Provides a flood protection queue for commands.
 */

/*!
    \fn void IrcCommandQueue::sizeChanged(int size)

    This signal is emitted when the queue \a size has changed.
 */

#ifndef IRC_DOXYGEN
IrcCommandQueuePrivate::IrcCommandQueuePrivate() : q_ptr(0),
    connection(0), batch(DEFAULT_BATCH), interval(DEFAULT_INTERVAL)
{
}

bool IrcCommandQueuePrivate::commandFilter(IrcCommand* cmd)
{
    Q_Q(IrcCommandQueue);
    if (cmd->type() == IrcCommand::Quit) {
        _irc_sendBatch(true);
    } else if (interval > 0 && !cmd->parent() && connection->isConnected()) {
        cmd->setParent(q);
        commands.enqueue(cmd);
        emit q->sizeChanged(commands.size());
        _irc_updateTimer();
        return true;
    }
    return false;
}

void IrcCommandQueuePrivate::_irc_updateTimer()
{
    if (connection && interval > 0 && !commands.isEmpty() && connection->isConnected()) {
        timer.setInterval(interval * 1000);
        if (!timer.isActive())
            timer.start();
    } else {
        if (timer.isActive())
            timer.stop();
    }
}

void IrcCommandQueuePrivate::_irc_sendBatch(bool force)
{
    Q_Q(IrcCommandQueue);
    if (!commands.isEmpty()) {
        int i = batch;
        while ((force || --i >= 0) && !commands.isEmpty()) {
            IrcCommand* cmd = commands.dequeue();
            if (cmd) {
                connection->sendCommand(cmd);
                cmd->deleteLater();
            }
        }
        emit q->sizeChanged(commands.size());
    }
    _irc_updateTimer();
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new command queue with \a parent.

    \note If \a parent is an instance of IrcConnection, it will be
    automatically assigned to \ref IrcCommandQueue::connection "connection".
 */
IrcCommandQueue::IrcCommandQueue(QObject* parent) : QObject(parent), d_ptr(new IrcCommandQueuePrivate)
{
    Q_D(IrcCommandQueue);
    d->q_ptr = this;
    connect(&d->timer, SIGNAL(timeout()), this, SLOT(_irc_sendBatch()));
    setConnection(qobject_cast<IrcConnection*>(parent));
}

/*!
    Destructs the command queue.
 */
IrcCommandQueue::~IrcCommandQueue()
{
    clear();
}

/*!
    This property holds the batch size.

    This is the amount of commands sent at once.
    The default value is \c 3.

    \par Access functions:
    \li int <b>batch</b>() const
    \li void <b>setBatch</b>(int batch)
 */
int IrcCommandQueue::batch() const
{
    Q_D(const IrcCommandQueue);
    return d->batch;
}

void IrcCommandQueue::setBatch(int batch)
{
    Q_D(IrcCommandQueue);
    d->batch = batch;
}

/*!
    This property holds the queue processing interval in seconds.

    The default value is \c 2 seconds. A value equal to or
    less than \c 0 seconds disables command queueing.

    \par Access functions:
    \li int <b>interval</b>() const
    \li void <b>setInterval</b>(int seconds)
 */
int IrcCommandQueue::interval() const
{
    Q_D(const IrcCommandQueue);
    return d->interval;
}

void IrcCommandQueue::setInterval(int seconds)
{
    Q_D(IrcCommandQueue);
    if (d->interval != seconds) {
        d->interval = seconds;
        d->_irc_updateTimer();
    }
}

/*!
    This property holds the current size of the queue.

    \par Access function:
    \li int <b>size</b>() const

    \par Notifier signal:
    \li void <b>sizeChanged</b>(int size)
 */
int IrcCommandQueue::size() const
{
    Q_D(const IrcCommandQueue);
    return d->commands.size();
}

/*!
    This property holds the associated connection.

    \par Access functions:
    \li IrcConnection* <b>connection</b>() const
    \li void <b>setConnection</b>(IrcConnection* connection)
 */
IrcConnection* IrcCommandQueue::connection() const
{
    Q_D(const IrcCommandQueue);
    return d->connection;
}

void IrcCommandQueue::setConnection(IrcConnection* connection)
{
    Q_D(IrcCommandQueue);
    if (d->connection != connection) {
        if (d->connection) {
            d->connection->removeCommandFilter(d);
            disconnect(d->connection, SIGNAL(connected()), this, SLOT(_irc_sendBatch()));
            disconnect(d->connection, SIGNAL(disconnected()), this, SLOT(_irc_updateTimer()));
        }
        d->connection = connection;
        if (connection) {
            connection->installCommandFilter(d);
            connect(connection, SIGNAL(connected()), this, SLOT(_irc_sendBatch()));
            connect(connection, SIGNAL(disconnected()), this, SLOT(_irc_updateTimer()));
        }
        d->_irc_updateTimer();
    }
}

/*!
    This methods clears the command queue.

    \note Any queued commands are not sent.

    \sa flush()
 */
void IrcCommandQueue::clear()
{
    Q_D(IrcCommandQueue);
    qDeleteAll(d->commands);
    d->commands.clear();
    d->_irc_updateTimer();
}

/*!
    This methods flushes the whole command queue without batching.
 */
void IrcCommandQueue::flush()
{
    Q_D(IrcCommandQueue);
    d->_irc_sendBatch(true);
}

#include "moc_irccommandqueue.cpp"
#include "moc_irccommandqueue_p.cpp"

IRC_END_NAMESPACE
