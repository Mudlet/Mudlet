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

#include "irc.h"
#include "irccore.h"
#include "irccommand.h"
#include "ircconnection.h"
#include "ircmessage_p.h"
#include <QMetaEnum>
#include <QDebug>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif // QT_NO_SSL

IRC_BEGIN_NAMESPACE

/*!
    \file irc.h
    \brief \#include &lt;Irc&gt;
 */

/*!
    \class Irc
    \ingroup core
    \brief Miscellaneous identifiers used throughout the library.
 */

/*!
    \since 3.5
    \property bool Irc::secureSupported
    This property holds whether SSL is supported.

    The value may be \c false for the following reasons:
    \li Qt was built without SSL support (\c QT_NO_SSL is defined), or
    \li The platform does not support SSL (QSslSocket::supportsSsl() returns \c false).

    \par Access function:
    \li static bool <b>isSecureSupported</b>()

    \sa IrcConnection::secure, QSslSocket::supportsSsl()
 */
bool Irc::isSecureSupported()
{
#ifdef QT_NO_SSL
    return false;
#else
    return QSslSocket::supportsSsl();
#endif
}

/*!
    \since 3.5

    This property holds the list of supported SASL (Simple Authentication and Security Layer) mechanisms.

    \par Access function:
    \li static QStringList <b>supportedSaslMechanisms</b>()

    \sa IrcConnection::saslMechanism, \ref ircv3
 */
QStringList Irc::supportedSaslMechanisms()
{
    return QStringList() << QLatin1String("PLAIN");
}

/*!
    \since 3.5

    This property holds the list of supported capabilities.

    These capabilities are guaranteed to be compatible with the framework. In order to
    easily enable all possible supported capabilities, assign Irc::supportedCapabilities
    to IrcNetwork::requestedCapabilities.

    \par Access function:
    \li static QStringList <b>supportedCapabilities</b>()

    \sa \ref capabilities, \ref ircv3
 */
QStringList Irc::supportedCapabilities()
{
    return QStringList() << QLatin1String("account-notify")
                         << QLatin1String("account-tag")
                         << QLatin1String("away-notify")
                         << QLatin1String("batch")
                         << QLatin1String("cap-notify")
                         << QLatin1String("chghost")
                         << QLatin1String("echo-message")
                         << QLatin1String("extended-join")
                         << QLatin1String("invite-notify")
                         << QLatin1String("multi-prefix")
                         << QLatin1String("sasl")
                         << QLatin1String("server-time")
                         << QLatin1String("userhost-in-names");
}

/*!
    Returns the version number of Communi at run-time as a string (for example, "1.2.3").
    This may be a different version than the version the application was compiled against.

    \sa IRC_VERSION, IRC_VERSION_STR
 */
QString Irc::version()
{
    return QLatin1String(IRC_VERSION_STR);
}

/*!
    Returns the numeric \a code as a string or a null string if the code is unknown.

    \sa Irc::Code, IrcNumericMessage::code()
 */
QString Irc::codeToString(int code)
{
    const int index = Irc::staticMetaObject.indexOfEnumerator("Code");
    QMetaEnum enumerator = Irc::staticMetaObject.enumerator(index);
    return QLatin1String(enumerator.valueToKey(code));
}

/*!
    Returns the nick part of the specified \a prefix.

    Nick part of a prefix as specified in RFC 1459:
    <pre>
    <b>&lt;nick&gt;</b> [ '!' &lt;ident&gt; ] [ '@' &lt;host&gt; ]
    </pre>

    \sa IrcMessage::prefix, IrcMessage::nick
 */
QString Irc::nickFromPrefix(const QString& prefix)
{
    QString nick;
    IrcMessagePrivate::parsePrefix(prefix, &nick, 0, 0);
    return nick;
}

/*!
    Returns the ident part of the specified \a prefix.

    Ident part of a prefix as specified in RFC 1459:
    <pre>
    &lt;nick&gt; [ '!' <b>&lt;ident&gt;</b> ] [ '@' &lt;host&gt; ]
    </pre>

    \sa IrcMessage::prefix, IrcMessage::ident
 */
QString Irc::identFromPrefix(const QString& prefix)
{
    QString ident;
    IrcMessagePrivate::parsePrefix(prefix, 0, &ident, 0);
    return ident;
}

/*!
    Returns the host part of the specified \a prefix.

    Host part of a prefix as specified in RFC 1459:
    <pre>
    &lt;nick&gt; [ '!' &lt;ident&gt; ] [ '@' <b>&lt;host&gt;</b> ]
    </pre>

    \sa IrcMessage::prefix, IrcMessage::host
 */
QString Irc::hostFromPrefix(const QString& prefix)
{
    QString host;
    IrcMessagePrivate::parsePrefix(prefix, 0, 0, &host);
    return host;
}

/*!
    \deprecated Use IrcCore::registerMetaTypes() instead.
 */
void Irc::registerMetaTypes()
{
    IrcCore::registerMetaTypes();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, Irc::Code code)
{
    const int index = Irc::staticMetaObject.indexOfEnumerator("Code");
    QMetaEnum enumerator = Irc::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(code);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, Irc::DataRole role)
{
    const int index = Irc::staticMetaObject.indexOfEnumerator("DataRole");
    QMetaEnum enumerator = Irc::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(role);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, Irc::Color color)
{
    const int index = Irc::staticMetaObject.indexOfEnumerator("Color");
    QMetaEnum enumerator = Irc::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(color);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, Irc::SortMethod method)
{
    const int index = Irc::staticMetaObject.indexOfEnumerator("SortMethod");
    QMetaEnum enumerator = Irc::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(method);
    debug << (key ? key : "Unknown");
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_irc.cpp"

IRC_END_NAMESPACE
