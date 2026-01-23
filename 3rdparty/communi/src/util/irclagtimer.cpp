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

#include "irclagtimer.h"
#include "irclagtimer_p.h"
#include "ircconnection.h"
#include "ircmessage.h"
#include "irccommand.h"
#include <QDateTime>

IRC_BEGIN_NAMESPACE

static const int DEFAULT_INTERVAL = 60;

/*!
    \file irclagtimer.h
    \brief \#include &lt;IrcLagTimer&gt;
 */

/*!
    \class IrcLagTimer irclagtimer.h <IrcLagTimer>
    \ingroup util
    \brief Provides a timer for measuring lag.

    \note IrcLagTimer relies on functionality introduced in Qt 4.7.0, and is
          therefore not functional when built against earlier versions of Qt.
 */

/*!
    \fn void IrcLagTimer::lagChanged(qint64 lag)

    This signal is emitted when the \a lag has changed.
 */

#ifndef IRC_DOXYGEN
IrcLagTimerPrivate::IrcLagTimerPrivate() :  interval(DEFAULT_INTERVAL)
{
}

bool IrcLagTimerPrivate::messageFilter(IrcMessage* msg)
{
    if (msg->type() == IrcMessage::Pong)
        return processPongReply(static_cast<IrcPongMessage*>(msg));
    return false;
}

bool IrcLagTimerPrivate::processPongReply(IrcPongMessage* msg)
{
#if QT_VERSION >= 0x040700
    // TODO: configurable format?
    if (msg->argument().startsWith(QLatin1String("communi/"))) {
        bool ok = false;
        qint64 timestamp = msg->argument().mid(8).toLongLong(&ok);
        if (ok) {
            --pendingPings;
            updateLag(QDateTime::currentMSecsSinceEpoch() - timestamp);
            return true;
        }
    }
#endif // QT_VERSION
    return false;
}

void IrcLagTimerPrivate::_irc_connected()
{
#if QT_VERSION >= 0x040700
    pendingPings = 0;
    if (interval > 0)
        timer.start();
#endif // QT_VERSION
}

void IrcLagTimerPrivate::_irc_pingServer()
{
#if QT_VERSION >= 0x040700
    // TODO: configurable format?
    QString cmd = QStringLiteral("PING communi/%1").arg(QDateTime::currentMSecsSinceEpoch());
    connection->sendData(cmd.toUtf8());
    qint64 pingLag = pendingPings * interval * 1000ll;
    if (lag > -1 && pingLag > lag)
        updateLag(pingLag);
    ++pendingPings;
#endif // QT_VERSION
}

void IrcLagTimerPrivate::_irc_disconnected()
{
#if QT_VERSION >= 0x040700
    updateLag(-1);
    pendingPings = 0;
    if (timer.isActive())
        timer.stop();
#endif // QT_VERSION
}

void IrcLagTimerPrivate::updateTimer()
{
#if QT_VERSION >= 0x040700
    if (connection && interval > 0) {
        timer.setInterval(interval * 1000);
        if (!timer.isActive() && connection->isConnected())
            timer.start();
    } else {
        if (timer.isActive())
            timer.stop();
        updateLag(-1);
    }
#endif // QT_VERSION
}

void IrcLagTimerPrivate::updateLag(qint64 value)
{
    Q_Q(IrcLagTimer);
    value = qMax(-1ll, value);
    if (lag != value) {
        lag = value;
        emit q->lagChanged(lag);
    }
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new lag timer with \a parent.

    \note If \a parent is an instance of IrcConnection, it will be
    automatically assigned to \ref IrcLagTimer::connection "connection".
 */
IrcLagTimer::IrcLagTimer(QObject* parent) : QObject(parent), d_ptr(new IrcLagTimerPrivate)
{
    Q_D(IrcLagTimer);
    d->q_ptr = this;
    connect(&d->timer, SIGNAL(timeout()), this, SLOT(_irc_pingServer()));
    setConnection(qobject_cast<IrcConnection*>(parent));
}

/*!
    Destructs the lag timer.
 */
IrcLagTimer::~IrcLagTimer()
{
}

/*!
    This property holds the associated connection.

    \par Access functions:
    \li IrcConnection* <b>connection</b>() const
    \li void <b>setConnection</b>(IrcConnection* connection)
 */
IrcConnection* IrcLagTimer::connection() const
{
    Q_D(const IrcLagTimer);
    return d->connection;
}

void IrcLagTimer::setConnection(IrcConnection* connection)
{
    Q_D(IrcLagTimer);
    if (d->connection != connection) {
        if (d->connection) {
            d->connection->removeMessageFilter(d);
            disconnect(d->connection, SIGNAL(connected()), this, SLOT(_irc_connected()));
            disconnect(d->connection, SIGNAL(disconnected()), this, SLOT(_irc_disconnected()));
        }
        d->connection = connection;
        if (connection) {
            connection->installMessageFilter(d);
            connect(connection, SIGNAL(connected()), this, SLOT(_irc_connected()));
            connect(connection, SIGNAL(disconnected()), this, SLOT(_irc_disconnected()));
        }
        d->updateLag(-1);
        d->updateTimer();
    }
}

/*!
    This property holds the current lag in milliseconds.

    The value is \c -1 when
    \li the connection is not connected,
    \li the lag has not yet been measured,
    \li the lag timer is disabled (interval <= 0s), or
    \li the Qt version is too old (4.7.0 or later is required).

    \par Access function:
    \li qint64 <b>lag</b>() const

    \par Notifier signal:
    \li void <b>lagChanged</b>(qint64 lag)
 */
qint64 IrcLagTimer::lag() const
{
    Q_D(const IrcLagTimer);
    return d->lag;
}

/*!
    This property holds the lag measurement interval in seconds.

    The default value is \c 60 seconds. A value equal to or
    less than \c 0 seconds disables the lag measurement.

    \par Access functions:
    \li int <b>interval</b>() const
    \li void <b>setInterval</b>(int seconds)
 */
int IrcLagTimer::interval() const
{
    Q_D(const IrcLagTimer);
    return d->interval;
}

void IrcLagTimer::setInterval(int seconds)
{
    Q_D(IrcLagTimer);
    if (d->interval != seconds) {
        d->interval = seconds;
        d->updateTimer();
    }
}

#include "moc_irclagtimer.cpp"
#include "moc_irclagtimer_p.cpp"

IRC_END_NAMESPACE
