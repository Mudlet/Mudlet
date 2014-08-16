/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#include "ircsender.h"
#include <QRegExp>

/*!
    \file ircsender.h
    \brief #include &lt;IrcSender&gt;
 */

/*!
    \class IrcSender ircsender.h <IrcSender>
    \ingroup utility
    \brief The IrcSender class is a helper class for handling IRC message sender prefixes.

    An IRC message sender prefix as specified in RFC 1459:
    <pre>
    &lt;prefix&gt; ::= &lt;servername&gt; | &lt;nick&gt; [ '!' &lt;user&gt; ] [ '@' &lt;host&gt; ]
    </pre>
 */

/*!
    \fn QString IrcSender::name() const
    Returns the name.

    <pre>
    &lt;prefix&gt; ::= <b>&lt;servername&gt;</b> | <b>&lt;nick&gt;</b> [ '!' &lt;user&gt; ] [ '@' &lt;host&gt; ]
    </pre>
 */

/*!
    \fn void IrcSender::setName(const QString& name)
    Sets the name.

    <pre>
    &lt;prefix&gt; ::= <b>&lt;servername&gt;</b> | <b>&lt;nick&gt;</b> [ '!' &lt;user&gt; ] [ '@' &lt;host&gt; ]
    </pre>
 */

/*!
    \fn QString IrcSender::user() const
    Returns the user.

    <pre>
    &lt;prefix&gt; ::= &lt;servername&gt; | &lt;nick&gt; [ '!' <b>&lt;user&gt;</b> ] [ '@' &lt;host&gt; ]
    </pre>
 */

/*!
    \fn void IrcSender::setUser(const QString& user)
    Sets the user.

    <pre>
    &lt;prefix&gt; ::= &lt;servername&gt; | &lt;nick&gt; [ '!' <b>&lt;user&gt;</b> ] [ '@' &lt;host&gt; ]
    </pre>
 */

/*!
    \fn QString IrcSender::host() const
    Returns the host.

    <pre>
    &lt;prefix&gt; ::= &lt;servername&gt; | &lt;nick&gt; [ '!' &lt;user&gt; ] [ '@' <b>&lt;host&gt;</b> ]
    </pre>
 */

/*!
    \fn void IrcSender::setHost(const QString& host)
    Sets the host.

    <pre>
    &lt;prefix&gt; ::= &lt;servername&gt; | &lt;nick&gt; [ '!' &lt;user&gt; ] [ '@' <b>&lt;host&gt;</b> ]
    </pre>
 */

/*!
    Constructs a new IrcSender, optionally initializing to \a prefix.
 */
IrcSender::IrcSender(const QString& prefix)
{
    setPrefix(prefix);
}

/*!
    Returns \c true if the sender is valid; otherwise \c false.

    A sender is considered valid if the name is not empty.
 */
bool IrcSender::isValid() const
{
    return !n.isEmpty();
}

/*!
    Returns the whole prefix.
 */
QString IrcSender::prefix() const
{
    if (!isValid())
        return QString();

    QString pfx = n;
    if (!u.isEmpty()) pfx += "!" + u;
    if (!h.isEmpty()) pfx += "@" + h;
    return pfx;
}

/*!
    Sets the whole prefix.

    \warning Overrides any existing name, user or host.
 */
void IrcSender::setPrefix(const QString& prefix)
{
    QRegExp rx("([^!@\\s]+)(![^@\\s]+)?(@\\S+)?");
    bool match = rx.exactMatch(prefix.trimmed());
    n = match ? rx.cap(1) : QString();
    u = match ? rx.cap(2).mid(1) : QString();
    h = match ? rx.cap(3).mid(1) : QString();
}
