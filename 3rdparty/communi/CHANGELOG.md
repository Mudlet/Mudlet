## [3.7.0] - 2021-12-18
- Fix build with Qt 6
- Add a way to skip capability validation
- Make sure to remove buffer from channel list when buffer get destroyed
- Don't reset connection count on connection close but on quit
- Support more responses to CAP LS 302

## [3.6.0] - 2020-10-29
- General
  - Fixed deprecation warnings up until Qt 5.15
  - Removed MPL-licensed (BSD-incompatible) code
    - A copy of uchardet-0.0.1, since MPL is not BSD-compatible.
      The system libuchardet is used via pkg-config (configure -uchardet).
    - Mozilla's MPL-rdf_utils.c - QTextCodec is used instead.
  - Improved uchardet & ICU selection and auto-detection.
  - Modernized the codebase to use nullptr, override, and default member init
  - Minor build system fixes
  - Added -(no)make qml configure option
- IrcCore
  - Added IrcConnection::connectionCount to count established connections
  - Fixed UTF-8 handling in IrcMessageDecoder::decode()
  - Fixed IrcProtocol to allow spaces in the PASS command
- IrcModel
  - IMPORTANT BEHAVIOR CHANGES
    - IrcBufferModel has been changed to deliver notice messages to
      the target buffer, and create the buffer if it does not exist.
  - Fixed IrcBufferModel to sort channels with keys before channels without
    keys, and group as many channels into a single join command as possible.
- IrcUtil
  - Fixed IrcTextFormat to exclude semi-colons while percent encoding URLs
  - Fixed IrcTextFormat to retain HTML entities
  - Fixed IrcLagTimer to count unanswered pings towards the lag

## [3.5.0] - 2016-11-18
- General
  - Added support for debug levels and filters
- IrcCore
  - Added IrcCommand::network
  - Added IrcBatchMessage
  - Added IrcConnection::ctcpReplies
  - Added IrcMessage::tag(name)
  - Added IrcMessage::setTag(name, value)
  - Added IrcMessage::parameter(index)
  - Added IrcMessage::setParameter(index, parameter)
  - Added IrcMessage::implicit
  - Added Irc::secureSupported
  - Added Irc::supportedSaslMechanisms
  - Deprecated IrcConnection::secureSupported
  - Deprecated IrcConnection::supportedSaslMechanisms
  - Added Irc::supportedCapabilities
  - Added IrcMessage::clone()
  - Added IrcMessage::testFlag()
  - Added IrcMessage::setFlag()
  - Added IrcWhoisMessage::awayReason

## [3.4.0] - 2015-07-19
- IrcCore
  - Added IrcConnection::clone()
  - Added IrcCommand::createMonitor()
  - Added IrcNetwork::statusPrefixes
  - Added IrcNetwork::MonitorCount enum value
  - Added IrcMessage::account
  - Added IrcPrivateMessage::statusPrefix
  - Added IrcNoticeMessage::statusPrefix
  - Added IrcHostChangeMessage
  - Added Irc::RPL_MONONLINE, RPL_MONOFFLINE, RPL_MONLIST, and
    RPL_ENDOFMONLIST enum values
  - Added support for the following IRCv3.2 extensions: account-tag,
    cap-notify, chghost, monitor, and server-time
- IrcModel
  - Added Irc::SortByActivity support to IrcBufferModel
  - Added IrcBufferModel::monitorEnabled
- IrcUtil
  - Added IrcCommandQueue

## [3.3.0] - 2014-12-29
- IrcCore
  - Added IrcConnection::nickNames
  - Added IrcConnection::servers
  - Added IrcCommand::connection
  - Added IrcMessage::Implicit
  - Added IrcInviteMessage::isReply()
    - NOTE: Notice that there is no need to catch RPL_INVITING or
            RPL_INVITED anymore. These numeric replies are now
            composed to IrcInviteMessage.
  - Added IrcJoinMessage::account and IrcJoinMessage::realName
    - NOTE: Only set if the extended-join capability is enabled.
  - Added IrcAccountMessage
    - NOTE: Only received if the account-notify capability is enabled.
  - Added IrcAwayMessage
    - NOTE: Notice that there is no need to catch RPL_AWAY, RPL_UNAWAY,
            or RPL_NOWAWAY anymore. These numeric replies are now
            composed to IrcAwayMessage. For other users, only received
            if the away-notify capability is enabled.
  - Added IrcNumericMessage::composed
  - Added IrcWhoisMessage and IrcWhowasMessage
- IrcModel
  - Added IrcChannel::who() slot for convenience
  - Added IrcBufferModel::joinDelay
  - Added IrcUserModel::titles

## [3.2.0] - 2014-08-09
- General
  - Relicensed to BSD
- Build system
  - Added support for no_install_xxx qmake configs
  - Exported qmake variables IRC_VERSION(_MAJOR/MINOR/PATCH)
- IrcCore
  - Introduced IrcProtocol
  - Exposed IrcConnection::protocol
  - Added IrcConnection::secureSupported
  - Added IrcConnection::secureError() [signal]
  - Restored IrcMessage::own for convenience
  - Added Playback to IrcMessage::Flags
  - Added IrcMessage::setFlags()
  - Added IrcCore::registerMetaTypes()
- IrcModel
  - Added IrcBufferModel::receiveMessage()
  - Added IrcModel::registerMetaTypes()
  - Handle IrcMessage::Playback as appropriate
- IrcUtil
  - Made IrcCommandParser retain whitespace and compose a single
    command parameter out of a multi-word input parameter
  - Made IrcCompleter prioritize channel users over buffer names
  - Added IrcCompleter::Direction
  - Added IrcUtil::registerMetaTypes()
  - Added IrcTextFormat::parse()
  - Added IrcTextFormat::plainText
  - Added IrcTextFormat::html
  - Added IrcTextFormat::urls

## [3.1.1] - 2014-05-17
- General
  - Fixed configure script's default imports and qml directories
  - Added missing namespace macros to the QML plugins
- IrcCore
  - Added missing enums to Irc::registerMetaTypes()
- IrcModel
  - Fixed IrcChannel::isChannel() for namespaced builds

## [3.1.0] - 2014-03-08
- General
  - IrcUtil now depends on IrcModel
- IrcCore
  - Added IrcConnection::userData
  - Added IrcConnection::saveState()
  - Added IrcConnection::restoreState()
  - Added IrcConnection::channelKeyRequired() [signal]
  - Added IrcConnection::nickNameRequired() [signal]
  - Deprecated IrcConnection::nickNameReserved() [signal]
  - Added IrcMessage::tags
  - Added IrcModeMessage::arguments()
  - Introduced IrcWhoReplyMessage
- IrcModel
  - Added IrcBufferModel::empty
  - Added IrcBufferModel::saveState()
  - Added IrcBufferModel::restoreState()
  - Added IrcBufferModel::persistent
  - Added IrcChannel::key
  - Added IrcChannel::join(QString key)
  - Added IrcBuffer::userData
  - Added IrcBuffer::close()
  - Added IrcUserModel::empty
  - Added IrcUser::away
  - Added IrcUser::servOp
- IrcUtil
  - Introduced IrcCompleter
  - Added IrcTextFormat::spanFormat
  - Improved IrcTextFormat::toHtml() performance on Qt 5 (QRegularExpression)
- Examples
  - Made the QtQuick example remember connection settings (requires Qt 5.2)

## [3.0.3] - 2014-05-17
- General
  - Fixed configure script's default imports and qml directories
  - Added missing namespace macros to the QML plugins
- IrcCore
  - Added missing enums to Irc::registerMetaTypes()
- IrcModel
  - Fixed IrcChannel::isChannel() for namespaced builds

## [3.0.2] - 2014-02-02
- General
  - Added missing QML plugin type info files
- IrcCore
  - Fix login when SASL enabled but not available
  - Fixed RPL_ISUPPORT handling when server sends ERR_NOMOTD
- IrcModel
  - Fixed IrcBufferModel to not block IrcBuffer signals on destruction/removal
  - Fixed IrcBufferModel to deliver own echoed messages to the target buffer
- IrcUtil
  - Fixed IrcTextFormat::toHtml() to not percent encode comma in URLs

## [3.0.1] - 2013-11-12
- IrcCore
  - Fixed IrcConnection::open() to bail out when already active
  - Fixed IrcModeMessage::kind() for modes with arguments
- IrcModel
  - Added missing IrcUserModel::channelChanged() to docs
  - Fixed a potential crash in IrcUserModel sorting
  - Fixed IrcChannel::isActive() on quit
  - Fixed IrcBufferModel to deliver messages only to active buffers
  - Fixed IrcUser mode & prefix sorting in the "ranking" order
- IrcUtil
  - Added missing export macro to IrcPalette
  - Fixed IrcTextFormat::toHtml() to percent encode special characters in URLs

## [3.0.0] - 2013-10-20
- General
  - Modularized: IrcCore, IrcModel & IrcUtil
  - Added namespace support
  - Full QML support & restored the QML plugin
  - Renamed all COMMUNI_XXX macros to IRC_XXX
  - Made uchardet the default encoding detection backend
- IrcCore
  - Added Irc::registerMetaTypes()
  - Renamed Irc::toString() to Irc::codeToString()
  - Added Irc::nick/ident/hostFromPrefix()
  - Renamed IrcSessionInfo to IrcNetwork
  - Renamed IrcSession to IrcConnection
  - Added IrcConnection::network
  - Added IrcConnection::enabled
  - Added IrcConnection::status
  - Added IrcConnection::displayName
  - Added IrcConnection::reconnectDelay
  - Replaced IrcConnection::password signal with a property
  - Added IrcConnection::saslMechanism & supportedSaslMechanisms
  - Added IrcConnection(host, parent) convenience constructor
  - Made IrcConnection::sendCommand() queue when inactive
  - Introduced IrcCommandFilter
  - Removed IrcSender
  - Replaced IrcMessage::sender with IrcMessage::(prefix|nick|ident|host)
  - Added IrcMessage::network
  - Added IrcPrivate/NoticeMessage::private property
  - Replaced IrcNickMessage::nick with oldNick & newNick
  - Renamed IrcPrivate/NoticeMessage::message to content
  - Replaced IrcMessage::fromCommand() with IrcCommand::toMessage()
- IrcModel
  - Added IrcUserModel::sortOrder & sortMethod
  - Added IrcUserModel::indexOf(IrcUser*)
  - Added IrcUserModel::clear()
  - Added IrcUser::title
  - Renamed IrcUserModel::user(QString) to find(QString)
  - Added IrcBufferModel::network
  - Added IrcBufferModel::sortOrder & sortMethod
  - Added IrcBufferModel::add(IrcBuffer*)
  - Added IrcBufferModel::indexOf(IrcBuffer*)
  - Added IrcBufferModel::remove(IrcBuffer*)
  - Added IrcBufferModel::buffer/channelPrototype
  - Split IrcBufferModel::create() to createBuffer() & createChannel()
  - Renamed IrcBufferModel::user(QString) to find(QString)
  - Removed IrcBufferModel::destroy()
  - Added IrcBuffer::persistent
  - Added IrcBuffer::sticky
  - Added IrcBuffer::network
  - Added IrcBuffer::receiveMessage()
- IrcUtil
  - Added IrcCommandParser::tolerant
  - Renamed IrcCommandParser::currentTarget to target
  - Replaced IrcCommandParser::prefix with triggers
  - Added details for IrcCommandParser::syntax()
  - Added IrcPalette color name properties
- Examples
  - Added a minimal example - connect, join & message in 8 lines of code
  - Added a Qt Quick based GUI client example
  - Added a bot example written in QML

## [2.2.0] - 2013-08-12
- Implemented SASL support (http://freenode.net/sasl)
- Introduced IrcBufferModel, IrcUserModel and IrcCommandParser
- Added new IrcSession convenience signals
  - void xxxMessageReceived(IrcXxxMessage* message)
  - void nickNameReserved(QString* alternate)
- Added bool IrcSession::secure property
- Added IrcSession::quit() slot for convenience
- Allowed constructing an invalid IrcSessionInfo
- Added IrcSessionInfo::channelModes(A|B|C|D)
- Added IrcModeMessage::Kind { Channel, User }
- Added IrcTopicMessage::isReply() and IrcModeMessage::isReply()
  - NOTE: Notice that there is no need to catch RPL_TOPIC, RPL_NOTOPIC
          or RPL_CHANNELMODEIS anymore. These numeric replies are now
          composed to IrcTopicMessage and IrcModeMessage.
- Made IrcLagTimer::session a writable property
- Improved submodule support:
  https://github.com/communi/libcommuni/wiki/Submodule
- Overall documentation improvements

## [2.1.1] - 2013-06-02
- Add missing docs for IrcMessage::Motd/Names enum values
- Enable using communi as a static lib & git submodule
- Fixed a memory leak in IrcProtocol
- Other cosmetic docs & build system cleanups and improvements

## [2.1.0] - 2013-05-19
- Introduced IrcMessageFilter and IrcLagTimer
- Added IrcSessionInfo::availableCapabilities() and activeCapabilities()
- Added QDebug stream operators for IrcSender
- Added QDataStream operators for IrcPalette and IrcTextFormat
- Added comparison operators for IrcSender, IrcPalette and IrcTextFormat
- Added IrcCommand::createPing()
- Added IrcMotdMessage and IrcNamesMessage
- Made IrcSession::sendCommand() only delete parentless commands
- Fixes and improvements to the congigure script

## [2.0.1] - 2013-04-26
- Fixed IrcMessage::toData()
- Fixed identify-msg capability handling
- Fixed ICU linking on 64-bit Windows
- Fixed IrcSession::close() to abort connecting

## [2.0.0] - 2013-03-08
- Focus on easy deployment
  - Removed all plugins
  - Better support for static builds and including(src.pri)
- Removed Symbian support
- The default FALLBACK encoding changed from UTF-8 to ISO-8859-15
  - UTF-8 did not make much sense as the default fallback encoding,
    since the fallback is only used when the message is detected NOT
    to be valid UTF-8 and the auto-detection fails
- Radically simplified examples
- Removed all deprecated classes and methods
- API changes:
  - Added:
    - IrcPalette
    - IrcTextFormat
    - IrcSessionInfo
    - IrcMessage::session
    - IrcMessage::timeStamp
    - IrcCommand::Type & IrcCommand::createXxx() for:
      - admin/info/knock/list/motd/stats/time/trace/users/version/who
  - Removed:
    - IrcUtil
      - use IrcTextFormat & IrcPalette instead
    - IrcCodecPlugin
      - use qmake -config icu or -config uchardet instead
    - IrcMessage::isOwn
      - use IrcMessage::flags() & IrcMessage::Own instead
    - IrcMessage::toString()
      - use IrcMessage::toData() instead
    - IrcMessage::fromString()
      - use IrcMessage::fromData/Command/Parameters() instead

## [1.2.2] - 2013-01-05
- Qt 5.0.0 final specific build fixes
- Fixed CTCP reply handling
- Fixed SSL connections

## [1.2.1] - 2012-12-03
- Fixed #24: IrcSessionPrivate::_q_error() is too verbose
- Fixed #25: IrcSessionPrivate::processLine() should not respond to
  CTCP requests
- Qt 5 specific build fixes
- Made the default fallback encoding ISO-8859-1
- Fixed a performance bottleneck in IrcDecoder::setEncoding()

## [1.2.0] - 2012-10-06
- Qt 5 support
- Implemented support for IRC capabilities as specified at
  http://www.leeh.co.uk/draft-mitchell-irc-capabilities-02.html
- Implemented support for the "identify-msg" capability
- Pluginized the message decoder
  - The default message decoder plugin is based on uchardet, which
    is built into the plugin, making it available on all platforms
  - An alternative message decoder plugin based on ICU is available
    in src/plugins/icu must be enabled/built by hand. It requires
    the presence of ICU (modify icu.pri if necessary) that is not
    available for all Communi supported platforms
  - The used message decoder plugin may be controlled by setting the
    COMMUNI_CODEC_PLUGIN (values: uchardet,icu) environment variable
  - In conjuction with the standard QT_PLUGIN_PATH, plugin paths may
    be controlled by setting the COMMUNI_PLUGIN_PATH environment
    variable
- API changes:
  - Added IrcCodecPlugin
  - Added QByteArray IrcMessage::encoding [property]
  - Added IrcMessage::Flags and IrcMessage::flags()
    - None, Own, Identified, Unidentified
  - Added IrcMessage::Capability and IrcCapabilityMessage
  - Added IrcCommand::Capability and IrcCommand::createCapability()
- Facelifted the desktop example

## [1.1.2] - 2012-09-05
- Docs:
  - Fixed IrcMessage::Private enum value to appear
- Examples:
  - Fixed settings to be remembered
  - Fixed a performance issue in channel message nick highlighting

## [1.1.1] - 2012-08-13
- Various build system fixes and improvements
  - Fixed shadow builds
  - Configure: improved qmake(-qt4) detection & added error handling
  - Fixed plugins, examples and tests to link to the local built libs
    instead of the installed ones whether they happen to exist
  - Added a 'no_rpath' qmake config
- Examples: fixed message formatting clash with nick names & URLs

## [1.1.0] - 2012-08-06
- Introduced a configure script
- Added a Symbian (QML) example
- Significantly revised the desktop example
  - Added dock icon badge, topic label & channel user list
  - Made it possible to emded parts into another app
- Replaced ICU with uchardet as the encoding detection engine
  - ICU is not available on all platforms, whereas uchardet is
    built into Communi and therefore available on all platforms
- IrcUtil::messageToHtml() to handle mIRC style background colors
- Detect message encoding part by part, instead of for the whole line
- Changed the semantics of IrcSession::encoding to a fallback encoding
- API changes:
  - Added bool IrcMessage::own [property]
  - Added IrcMessage::fromData() - deprecated IrcMessage::fromString()
  - Added IrcMessage::toData() - deprecated IrcMessage::toString()
  - Added QByteArray IrcCommand::encoding [property]
  - Added bool IrcSession::sendData(const QByteArray& data)

## [1.0.0] - 2011-11-11
- Renamed Communi (was LibIrcClient-Qt)
- Underwent a major rewrite
  - Split the monolithic IrcSession class
  - New event/message & property based IrcSession API
  - Removed the problematic buffer concept
  - New public classes: IrcCommand and IrcMessage & subclasses
- Improved QML compatibility
  - QObject based messages & commands
  - Used properties, signals, slots and invokables
  - Provided a declarative plugin: import Communi 1.0
- Environment variable COMMUNI_DEBUG=1 to help debugging - log socket state
  changes and received messages to the debug output
- Added desktop (QWidgets) & MeeGo (QML) examples

## [0.5.0] - 2010-11-05
- Irc::Session API additions:
  - addBuffer() and buffers() for buffer management
  - welcomed() signal, emitted when 001 is received
- Added IRC capability support
- Implemented IDENTIFY-MSG capability
- Fixed "icu.pri: Unescaped backslashes are deprecated"
- Remove buffer upon parting a channel
- Fixed Session::raw()'s check of written bytes
- Fixed #3 An incorrect buffer is created when StripNicks is off and
  EchoMessages
- Improved building on Mac
  - Make a difference between frameworks and standard dylibs (depending
    on Qt's installation style)
  - Set INSTALL_NAME correctly

## [0.4.1] - 2010-07-02
- Added support for static builds (qmake -config static)
- Fixed qmake project messages output only once
- Fixed MOC_DIR in release mode
- Added default MacPorts paths for ICU
- Fixed notices and messages from "real host" vs. "connected host" to get
  directed to the same buffer
- Fixed CTCP requests not to create a new buffer

## [0.4.0] - 2010-05-08
- Introduced Irc::Buffer - the concept of server/channel/query specific buffers
- Deprecated buffer-specific functionality in Irc::Session
- Fixed tabs not to cause underlined text
- Added a version number to the library
- Added Irc::Rfc::toString()
- Fixed euIRC connection problems

## [0.3.2] - 2009-08-30
- Fixed problems with "unknown" messages
- Fixed Irc::Session::connectSlotsByName() not to try to establish
  a connection when the parameter types don't match
- Code improvements

## [0.3.1] - 2009-04-10
- Fixed compilation on Mac
- Fixed Irc::Session::connected() and disconnected() to be emitted correctly
- Fixed Irc::Session::connectSlotsByName() not to establish multiple
  connections when a slot exists multiple times (reimplemented)
- Made Irc::Session::cmdMessage(), cmdNotice() and cmdCtcpAction() to
  emit own messages, notices and actions. This simplifies client
  implementation
- Started working on auto tests and added code coverage support
- Added a workaround for older Qt versions that don't have the
  QT_FORWARD_DECLARE_CLASS() macro defined

## [0.3.0] - 2009-02-06
- Added SSL support
- Quality & compatibility
  - no_keywords
  - QT_NO_CAST_FROM_ASCII
  - QT_NO_CAST_TO_ASCII
- Bug fixes & code improvements

## [0.2.0] - 2009-01-12
- The first fully Qt-based version

## [0.1.1] - 2008-12-17
- Added support for optional encoding detection with ICU

## [0.1.0] - 2008-11-15
- The first internal release
