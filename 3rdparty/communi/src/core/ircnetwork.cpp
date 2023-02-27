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

#include "ircnetwork.h"
#include "ircnetwork_p.h"
#include "ircconnection_p.h"
#include "ircprotocol.h"
#include "ircconnection.h"
#include "irccommand.h"
#include "irccore_p.h"
#include <QMetaEnum>
#include <QPointer>

IRC_BEGIN_NAMESPACE

/*!
    \file ircnetwork.h
    \brief \#include &lt;IrcNetwork&gt;
 */

/*!
    \class IrcNetwork ircnetwork.h IrcNetwork
    \ingroup core
    \brief Provides network information and capability management.

    \section info Network information

    IrcNetwork provides various information about the IRC network of a
    \ref IrcConnection::network "connection". This includes the \ref name
    "network name", supported \ref channelTypes "channel types", channel
    user \ref modes "mode characters" and \ref prefixes "prefix letters"
    and \ref statusPrefixes "status prefixes", and various \ref numericLimit
    "numeric limitations", such as the maximum nick, channel, topic and
    message lengths.

    Furthermore, IrcNetwork provides convenient methods for converting channel user
    \ref modeToPrefix() "modes to prefixes" and \ref prefixToMode() "vice versa" and
    testing whether a target name \ref isChannel() "is a channel".

    \note Most properties have empty values until the network
    information has been \ref initialized.

    \section capabilities Capability management

    IrcNetwork also provides means for capability management. It maintais a
    list of \ref availableCapabilities "available" and \ref activeCapabilities
    "active" capabilities, automatically \ref requestedCapabilities "requests"
    desired capabilities, and provides convenient methods for \ref requestCapability()
    "manual capability requests".

    \sa IrcConnection::network, Irc::supportedCapabilities, \ref ircv3
 */

/*!
    \fn void IrcNetwork::requestingCapabilities()

    This signal is emitted when capabilities are being requested.

    Normally it is enough to add the desired capabilities to the
    list of \ref requestedCapabilities "requested capabilities".
    Connect to this signal in order to implement more advanced
    capability handling eg. based on which capabilities are \ref
    availableCapabilities "available".

    \sa requestedCapabilities, availableCapabilities
 */

/*!
    \enum IrcNetwork::ModeType
    This enum describes the channel mode types.
 */

/*!
    \var IrcNetwork::TypeA
    \brief Type A modes

    Modes that add or remove an address to or from a list.
    These modes always take a parameter when sent by the server to a
    client; when sent by a client, they may be specified without a
    parameter, which requests the server to display the current
    contents of the corresponding list on the channel to the client.
 */

/*!
    \var IrcNetwork::TypeB
    \brief Type B modes

    Modes that change a setting on the channel. These modes
    always take a parameter.
 */

/*!
    \var IrcNetwork::TypeC
    \brief Type C modes

    Modes that change a setting on the channel. These modes
    take a parameter only when set; the parameter is absent when the
    mode is removed both in the client's and server's MODE command.
 */

/*!
    \var IrcNetwork::TypeD
    \brief Type D modes

    Modes that change a setting on the channel. These modes
    never take a parameter.
 */

/*!
    \var IrcNetwork::AllTypes
    \brief All type modes
 */

/*!
    \enum IrcNetwork::Limit
    This enum describes the numeric limit types.
 */

/*!
    \var IrcNetwork::NickLength
    \brief The maximum nick name length
 */

/*!
    \var IrcNetwork::ChannelLength
    \brief The maximum channel name length
 */

/*!
    \var IrcNetwork::TopicLength
    \brief The maximum channel topic length
 */

/*!
    \var IrcNetwork::MessageLength
    \brief The maximum message length
 */

/*!
    \var IrcNetwork::KickReasonLength
    \brief The maximum kick reason length
 */

/*!
    \var IrcNetwork::AwayReasonLength
    \brief The maximum away reason length
 */

/*!
    \var IrcNetwork::ModeCount
    \brief The maximum number of channel modes allowed per mode command
 */

/*!
    \since 3.4
    \var IrcNetwork::MonitorCount
    \brief The maximum amount of targets a client may have in their monitor list
 */

#ifndef IRC_DOXYGEN
IrcNetworkPrivate::IrcNetworkPrivate() :
    modes(QStringList() << "o" << "v"), prefixes(QStringList() << "@" << "+"), channelTypes("#")
{
}

static QHash<QString, int> numericValues(const QString& parameter)
{
    QHash<QString, int> values;
    const QStringList keyValues = parameter.split(",", Qt::SkipEmptyParts);
    foreach (const QString& keyValue, keyValues)
        values.insert(keyValue.section(":", 0, 0), keyValue.section(":", 1, 1).toInt());
    return values;
}

void IrcNetworkPrivate::setInfo(const QHash<QString, QString>& info)
{
    Q_Q(IrcNetwork);
    if (info.contains("NETWORK"))
        setName(info.value("NETWORK"));
    if (info.contains("PREFIX")) {
        const QString pfx = info.value("PREFIX");
        setModes(pfx.mid(1, pfx.indexOf(')') - 1).split("", Qt::SkipEmptyParts));
        setPrefixes(pfx.mid(pfx.indexOf(')') + 1).split("", Qt::SkipEmptyParts));
    }
    if (info.contains("CHANTYPES"))
        setChannelTypes(info.value("CHANTYPES").split("", Qt::SkipEmptyParts));
    if (info.contains("STATUSMSG"))
        setStatusPrefixes(info.value("STATUSMSG").split("", Qt::SkipEmptyParts));

    // TODO:
    if (info.contains("NICKLEN"))
        numericLimits.insert("NICKLEN", info.value("NICKLEN").toInt());
    if (info.contains("CHANNELLEN"))
        numericLimits.insert("CHANNELLEN", info.value("CHANNELLEN").toInt());
    if (info.contains("TOPICLEN"))
        numericLimits.insert("TOPICLEN", info.value("TOPICLEN").toInt());
    if (info.contains("KICKLEN"))
        numericLimits.insert("KICKLEN", info.value("KICKLEN").toInt());
    if (info.contains("AWAYLEN"))
        numericLimits.insert("AWAYLEN", info.value("AWAYLEN").toInt());
    if (info.contains("MODES"))
        numericLimits.insert("MODES", info.value("MODES").toInt());
    if (info.contains("MONITOR"))
        numericLimits.insert("MONITOR", info.value("MONITOR").toInt());
    if (info.contains("CHANMODES"))
        channelModes = info.value("CHANMODES").split(",", Qt::SkipEmptyParts);
    if (info.contains("MAXLIST"))
        modeLimits = numericValues(info.value("MAXLIST"));
    if (info.contains("CHANLIMIT"))
        channelLimits = numericValues(info.value("CHANLIMIT"));
    if (info.contains("TARGMAX"))
        targetLimits = numericValues(info.value("TARGMAX"));

    if (!initialized) {
        initialized = true;
        emit q->initialized();
    }
}

void IrcNetworkPrivate::setAvailableCapabilities(const QSet<QString>& capabilities)
{
    Q_Q(IrcNetwork);
    if (availableCaps != capabilities) {
        availableCaps = capabilities;
        emit q->availableCapabilitiesChanged(IrcPrivate::setToList(availableCaps));
    }
}

void IrcNetworkPrivate::setActiveCapabilities(const QSet<QString>& capabilities)
{
    Q_Q(IrcNetwork);
    if (activeCaps != capabilities) {
        activeCaps = capabilities;
        emit q->activeCapabilitiesChanged(IrcPrivate::setToList(activeCaps));
    }
}

void IrcNetworkPrivate::setName(const QString& value)
{
    Q_Q(IrcNetwork);
    if (name != value) {
        name = value;
        emit q->nameChanged(value);
    }
}

void IrcNetworkPrivate::setModes(const QStringList& value)
{
    Q_Q(IrcNetwork);
    if (modes != value) {
        modes = value;
        emit q->modesChanged(value);
    }
}

void IrcNetworkPrivate::setPrefixes(const QStringList& value)
{
    Q_Q(IrcNetwork);
    if (prefixes != value) {
        prefixes = value;
        emit q->prefixesChanged(value);
    }
}

void IrcNetworkPrivate::setChannelTypes(const QStringList& value)
{
    Q_Q(IrcNetwork);
    if (channelTypes != value) {
        channelTypes = value;
        emit q->channelTypesChanged(value);
    }
}

void IrcNetworkPrivate::setStatusPrefixes(const QStringList& value)
{
    Q_Q(IrcNetwork);
    if (statusPrefixes != value) {
        statusPrefixes = value;
        emit q->statusPrefixesChanged(value);
    }
}

QString IrcNetworkPrivate::getPrefix(const QString& str, const QStringList& prefixes)
{
    int i = 0;
    while (i < str.length() && prefixes.contains(str.at(i)))
        ++i;
    return str.left(i);
}

QString IrcNetworkPrivate::removePrefix(const QString& str, const QStringList& prefixes)
{
    int i = 0;
    while (i < str.length() && prefixes.contains(str.at(i)))
        ++i;
    return str.mid(i);
}
#endif // IRC_DOXYGEN

/*!
    \internal
    Constructs a new network object for IRC \a connection.
 */
IrcNetwork::IrcNetwork(IrcConnection* connection) : QObject(connection), d_ptr(new IrcNetworkPrivate)
{
    Q_D(IrcNetwork);
    d->q_ptr = this;
    d->connection = connection;
}

/*!
    \internal
    Destructs the IRC network.
 */
IrcNetwork::~IrcNetwork()
{
}

/*!
    \property bool IrcNetwork::initialized
    This property holds whether the network information has been initialized.

    Most properties have empty values until the server provides the
    relevant network information during the client-server handshake.

    \par Access function:
    \li bool <b>isInitialized</b>() const

    \par Notifier signal:
    \li void <b>initialized</b>()
 */
bool IrcNetwork::isInitialized()
{
    Q_D(const IrcNetwork);
    return d->initialized;
}

/*!
    This property holds the network name.

    \par Access function:
    \li QString <b>name</b>() const

    \par Notifier signal:
    \li void <b>nameChanged</b>(const QString& name)
 */
QString IrcNetwork::name() const
{
    Q_D(const IrcNetwork);
    return d->name;
}

/*!
    This property holds the supported channel user mode letters.

    Examples of typical channel user modes:
    Description      | Mode       | Prefix
    -----------------|------------|-------
    Channel operator | \b o       | @
    Voiced user      | \b v       | +

    \par Access function:
    \li QStringList <b>modes</b>() const

    \par Notifier signal:
    \li void <b>modesChanged</b>(const QStringList& modes)

    \sa prefixes, modeToPrefix()
 */
QStringList IrcNetwork::modes() const
{
    Q_D(const IrcNetwork);
    return d->modes;
}

/*!
    This property holds the supported channel user mode prefix characters.

    Examples of typical channel user modes:
    Description      | Mode       | Prefix
    -----------------|------------|-------
    Channel operator | o          | \b @
    Voiced user      | v          | \b +

    \par Access function:
    \li QStringList <b>prefixes</b>() const

    \par Notifier signal:
    \li void <b>prefixesChanged</b>(const QStringList& prefixes)

    \sa modes, prefixToMode(), statusPrefixes
 */
QStringList IrcNetwork::prefixes() const
{
    Q_D(const IrcNetwork);
    return d->prefixes;
}

/*!
    Converts a channel user mode letter to a prefix character.

    \sa modes, prefixToMode()
 */
QString IrcNetwork::modeToPrefix(const QString& mode) const
{
    Q_D(const IrcNetwork);
    return d->prefixes.value(d->modes.indexOf(mode));
}

/*!
    Converts a channel mode prefix character to a mode letter.

    \sa prefixes, modeToPrefix()
 */
QString IrcNetwork::prefixToMode(const QString& prefix) const
{
    Q_D(const IrcNetwork);
    return d->modes.value(d->prefixes.indexOf(prefix));
}

/*!
    This property holds the supported channel type prefix characters.

    Examples of typical channel types:
    Description      | Type | Example
    -----------------|------|---------
    Normal channel   | \#   | \#communi
    Local channel    | &    | &foo

    \par Access function:
    \li QStringList <b>channelTypes</b>() const

    \par Notifier signal:
    \li void <b>channelTypesChanged</b>(const QStringList& types)
 */
QStringList IrcNetwork::channelTypes() const
{
    Q_D(const IrcNetwork);
    return d->channelTypes;
}

/*!
    \since 3.4

    This property holds the supported message status prefixes.

    The server supports messaging channel members who have a certain status or higher.

    \par Access function:
    \li QStringList <b>statusPrefixes</b>() const

    \par Notifier signal:
    \li void <b>statusPrefixesChanged</b>(const QStringList& prefixes)

    \sa prefixes
 */
QStringList IrcNetwork::statusPrefixes() const
{
    Q_D(const IrcNetwork);
    return d->statusPrefixes;
}

/*!
    Returns \c true if the \a name is a channel.

    \code
    QString name = ...;
    if (connection->network()->isChannel(name))
        doSomeChannelAction(name);
    \endcode

    \sa channelTypes
 */
bool IrcNetwork::isChannel(const QString& name) const
{
    Q_D(const IrcNetwork);
    QString unprefixed = d->removePrefix(name, d->statusPrefixes);
    return !unprefixed.isEmpty() && d->channelTypes.contains(unprefixed.at(0));
}

/*!
    Returns the supported channel modes for specified \a types.

    \sa ModeType
 */
QStringList IrcNetwork::channelModes(IrcNetwork::ModeTypes types) const
{
    Q_D(const IrcNetwork);
    QStringList modes;
    if (types & TypeA)
        modes += d->channelModes.value(0).split("", Qt::SkipEmptyParts);
    if (types & TypeB)
        modes += d->channelModes.value(1).split("", Qt::SkipEmptyParts);
    if (types & TypeC)
        modes += d->channelModes.value(2).split("", Qt::SkipEmptyParts);
    if (types & TypeD)
        modes += d->channelModes.value(3).split("", Qt::SkipEmptyParts);
    return modes;
}

/*!
    Returns a numeric type of \a limit, or \c -1 if the limitation is not known.

    \sa modeLimit(), channelLimit(), targetLimit()
 */
int IrcNetwork::numericLimit(Limit limit) const
{
    Q_D(const IrcNetwork);
    QString key;
    switch (limit) {
        case NickLength:        key = QLatin1String("NICKLEN"); break;
        case ChannelLength:     key = QLatin1String("CHANNELLEN"); break;
        case TopicLength:       key = QLatin1String("TOPICLEN"); break;
        case MessageLength:     return 512; // RFC 1459
        case KickReasonLength:  key = QLatin1String("KICKLEN"); break;
        case AwayReasonLength:  key = QLatin1String("AWAYLEN"); break;
        case ModeCount:         key = QLatin1String("MODES"); break;
        case MonitorCount:      key = QLatin1String("MONITOR"); break;
    }
    return d->numericLimits.value(key, -1);
}

/*!
    Returns the limit of entries in the list per \a mode, or \c -1 if the limitation is not known.

    \sa modes()
 */
int IrcNetwork::modeLimit(const QString& mode) const
{
    Q_D(const IrcNetwork);
    return d->modeLimits.value(mode);
}

/*!
    Returns the limit for a \a type of channels, or \c -1 if the limitation is not known.

    \sa channelTypes()
 */
int IrcNetwork::channelLimit(const QString& type) const
{
    Q_D(const IrcNetwork);
    return d->channelLimits.value(type);
}

/*!
    Returns the limit of targets for a \a command, or \c -1 if the limitation is not known.
 */
int IrcNetwork::targetLimit(const QString& command) const
{
    Q_D(const IrcNetwork);
    return d->targetLimits.value(command);
}

/*!
    This property holds the available capabilities.

    \par Access function:
    \li QStringList <b>availableCapabilities</b>() const

    \par Notifier signal:
    \li void <b>availableCapabilitiesChanged</b>(const QStringList& capabilities)

    \sa requestedCapabilities, activeCapabilities
 */
QStringList IrcNetwork::availableCapabilities() const
{
    Q_D(const IrcNetwork);
    return IrcPrivate::setToList(d->availableCaps);
}

/*!
    This property holds the active capabilities.

    \par Access function:
    \li QStringList <b>activeCapabilities</b>() const

    \par Notifier signal:
    \li void <b>activeCapabilitiesChanged</b>(const QStringList& capabilities)

    \sa requestedCapabilities, availableCapabilities
 */
QStringList IrcNetwork::activeCapabilities() const
{
    Q_D(const IrcNetwork);
    return IrcPrivate::setToList(d->activeCaps);
}

/*!
    Returns \c true if the \a capability is \b available.

    \sa availableCapabilities
 */
bool IrcNetwork::hasCapability(const QString& capability) const
{
    Q_D(const IrcNetwork);
    return d->availableCaps.contains(capability);
}

/*!
    Returns \c true if the \a capability is \b active.

    \sa activeCapabilities
 */
bool IrcNetwork::isCapable(const QString& capability) const
{
    Q_D(const IrcNetwork);
    return d->activeCaps.contains(capability);
}

/*!
    Requests the specified \a capability.

    \note The \a capability is NOT added to the list of \ref requestedCapabilities
          "requested capabilities" to avoid them "piling up".
 */
bool IrcNetwork::requestCapability(const QString& capability)
{
    Q_D(IrcNetwork);
    if (d->connection)
        return d->connection->sendCommand(IrcCommand::createCapability(QLatin1String("REQ"), capability));
    return false;
}

/*!
    Requests the specified \a capabilities.

    \note The \a capabilities are NOT added to the list of \ref requestedCapabilities
          "requested capabilities" to avoid them "piling up".
 */
bool IrcNetwork::requestCapabilities(const QStringList& capabilities)
{
    Q_D(IrcNetwork);
    if (d->connection && d->connection->isActive())
        return d->connection->sendCommand(IrcCommand::createCapability(QLatin1String("REQ"), capabilities));
    return false;
}

/*!
    This property holds the requested capabilities.

    These capabilities are automatically requested during the handshake,
    right after requestingCapabilities() has been emitted.

    \par Access functions:
    \li QStringList <b>requestedCapabilities</b>() const
    \li void <b>setRequestedCapabilities</b>(const QStringList& capabilities)

    \par Notifier signal:
    \li void <b>requestedCapabilitiesChanged</b>(const QStringList& capabilities)

    \sa availableCapabilities, activeCapabilities
 */
QStringList IrcNetwork::requestedCapabilities() const
{
    Q_D(const IrcNetwork);
    return IrcPrivate::setToList(d->requestedCaps);
}


/*!
    This property specifies whether we should request capabilities right away
    after establishing connecting or not. Otherwise, we wait for CAP * LS and
    check if capabilities we're about to request are supported by the server.

    By default this is set to false

    \par Access functions:
    \li bool <b>skipCapabilityValidation</b>() const
    \li void <b>setSkipCapabilityValidation</b>(bool skip)

    \par Notifier signal:
    \li void <b>skipCapabilityValidationChanged</b>(bool skip)
 */

bool IrcNetwork::skipCapabilityValidation() const
{
    Q_D(const IrcNetwork);
    return d->skipCapabilityValidation;
}

void IrcNetwork::setSkipCapabilityValidation(bool skip)
{
    Q_D(IrcNetwork);
    if (d->skipCapabilityValidation != skip) {
        d->skipCapabilityValidation = skip;
        emit skipCapabilityValidationChanged(skip);
    }
}

void IrcNetwork::setRequestedCapabilities(const QStringList& capabilities)
{
    Q_D(IrcNetwork);
    const QSet<QString> caps = IrcPrivate::listToSet(capabilities);
    if (d->requestedCaps != caps) {
        d->requestedCaps = caps;
        emit requestedCapabilitiesChanged(IrcPrivate::setToList(caps));
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, IrcNetwork::Limit limit)
{
    const int index = IrcNetwork::staticMetaObject.indexOfEnumerator("Limit");
    QMetaEnum enumerator = IrcNetwork::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(limit);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, IrcNetwork::ModeType type)
{
    const int index = IrcNetwork::staticMetaObject.indexOfEnumerator("ModeType");
    QMetaEnum enumerator = IrcNetwork::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(type);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, IrcNetwork::ModeTypes types)
{
    QStringList lst;
    if (types == IrcNetwork::AllTypes) {
        lst << "AllTypes";
    } else {
        if (types & IrcNetwork::TypeA)
            lst << "TypeA";
        if (types & IrcNetwork::TypeB)
            lst << "TypeB";
        if (types & IrcNetwork::TypeC)
            lst << "TypeC";
        if (types & IrcNetwork::TypeD)
            lst << "TypeD";
    }
    debug.nospace() << '(' << qPrintable(lst.join("|")) << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const IrcNetwork* network)
{
    if (!network)
        return debug << "IrcNetwork(0x0) ";
    debug.nospace() << network->metaObject()->className() << '(' << (void*) network;
    if (!network->objectName().isEmpty())
        debug.nospace() << ", name=" << qPrintable(network->objectName());
    if (!network->name().isEmpty())
        debug.nospace() << ", network=" << qPrintable(network->name());
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircnetwork.cpp"

IRC_END_NAMESPACE
