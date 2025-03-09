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

#include "ircchannel.h"
#include "ircchannel_p.h"
#include "ircuser.h"
#include "ircuser_p.h"
#include <qdebug.h>

IRC_BEGIN_NAMESPACE

/*!
    \file ircuser.h
    \brief \#include &lt;IrcUser&gt;
 */

/*!
    \class IrcUser ircuser.h <IrcUser>
    \ingroup models
    \brief Keeps track of user status on a channel.

    \sa IrcUserModel
*/

#ifndef IRC_DOXYGEN
void IrcUserPrivate::setName(const QString& n)
{
    Q_Q(IrcUser);
    if (name != n) {
        name = n;
        emit q->nameChanged(name);
        emit q->titleChanged(q->title());
    }
}

void IrcUserPrivate::setPrefix(const QString& p)
{
    Q_Q(IrcUser);
    if (prefix != p) {
        prefix = p;
        emit q->prefixChanged(prefix);
        emit q->titleChanged(q->title());
    }
}

void IrcUserPrivate::setMode(const QString& m)
{
    Q_Q(IrcUser);
    if (mode != m) {
        mode = m;
        emit q->modeChanged(mode);
    }
}

void IrcUserPrivate::setServOp(const bool& o)
{
    Q_Q(IrcUser);
    if (servOp != o) {
        servOp = o;
        emit q->servOpChanged(servOp);
    }
}

void IrcUserPrivate::setAway(const bool& a)
{
    Q_Q(IrcUser);
    if (away != a) {
        away = a;
        emit q->awayChanged(away);
    }
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new user with \a parent.
 */
IrcUser::IrcUser(QObject* parent)
    : QObject(parent), d_ptr(new IrcUserPrivate)
{
    Q_D(IrcUser);
    d->q_ptr = this;
    d->channel = nullptr;
    d->away = false;
    d->servOp = false;
}

/*!
    Destructs the user object.
 */
IrcUser::~IrcUser()
{
}

/*!
    This property holds the title.

    The title consists of \ref prefix and \ref name.

    \par Access function:
    \li QString <b>title</b>() const

    \par Notifier signal:
    \li void <b>titleChanged</b>(const QString& title)
 */
QString IrcUser::title() const
{
    Q_D(const IrcUser);
    return d->prefix.left(1) + d->name;
}

/*!
    This property holds the name.

    \par Access function:
    \li QString <b>name</b>() const

    \par Notifier signal:
    \li void <b>nameChanged</b>(const QString& name)
 */
QString IrcUser::name() const
{
    Q_D(const IrcUser);
    return d->name;
}

/*!
    This property holds the prefix character.

    Typical prefix characters are \c @ (op) and \c + (voice).

    \note The prefix may be multiple characters if the \c multi-prefix
          capability is enabled.

    \par Access function:
    \li QString <b>prefix</b>() const

    \par Notifier signal:
    \li void <b>prefixChanged</b>(const QString& prefix)

    \sa mode, \ref ircv3
 */
QString IrcUser::prefix() const
{
    Q_D(const IrcUser);
    return d->prefix;
}

/*!
    This property holds the mode letter.

    Typical mode letters are \c o (op) and \c v (voice).

    \note The mode may be multiple characters if the \c multi-prefix
          capability is enabled.

    \par Access function:
    \li QString <b>mode</b>() const

    \par Notifier signal:
    \li void <b>modeChanged</b>(const QString& mode)

    \sa prefix, \ref ircv3
 */
QString IrcUser::mode() const
{
    Q_D(const IrcUser);
    return d->mode;
}

/*!
    \since 3.1

    \property bool IrcUser::servOp
    This property holds whether the user is a server operator.

    \note IRC servers do not send this information by default.
    In order to fetch the information for all users on a channel,
    issue a WHO command on the channel:
    \code
    IrcChannel* channel = user->channel();
    IrcCommand* command = IrcCommand::createWho(channel->title());
    channel->sendCommand(command);
    \endcode

    \par Access function:
    \li bool <b>isServOp</b>() const

    \par Notifier signal:
    \li void <b>servOpChanged</b>(bool servOp)
 */
bool IrcUser::isServOp() const
{
    Q_D(const IrcUser);
    return d->servOp;
}

/*!
    \since 3.1

    \property bool IrcUser::away
    This property holds whether the user is marked as being away.

    \note IRC servers do not send this information by default.
    In order to fetch the information for all users on a channel,
    issue a WHO command on the channel:
    \code
    IrcChannel* channel = user->channel();
    IrcCommand* command = IrcCommand::createWho(channel->title());
    channel->sendCommand(command);
    \endcode

    \par Access function:
    \li bool <b>isAway</b>() const

    \par Notifier signal:
    \li void <b>awayChanged</b>(bool away)
 */
bool IrcUser::isAway() const
{
    Q_D(const IrcUser);
    return d->away;
}

/*!
    This property holds the channel of the user.

    \par Access function:
    \li \ref IrcChannel* <b>channel</b>() const
 */
IrcChannel* IrcUser::channel() const
{
    Q_D(const IrcUser);
    return d->channel;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const IrcUser* user)
{
    if (!user)
        return debug << "IrcUser(0x0) ";
    debug.nospace() << user->metaObject()->className() << '(' << (void*) user;
    if (!user->objectName().isEmpty())
        debug.nospace() << ", name=" << qPrintable(user->objectName());
    if (!user->name().isEmpty())
        debug.nospace() << ", user=" << qPrintable(user->name());
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircuser.cpp"

IRC_END_NAMESPACE
