/*
* Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
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

#include "../include/irc.h"
#include <QMetaEnum>

/*! \mainpage LibIrcClient-Qt - a cross-platform C++ IRC library

    \section Introduction

    LibIrcClient-Qt is a cross-platform IRC client library written with Qt 4.

    \section Installation

    To compile LibIrcClient-Qt, do the standard mantra:
    \code
    # qmake -config (debug|release)
    # make
    # sudo make install
    \endcode

    \section Usage

    Add the following line to your qmake .pro file:
    \code
    CONFIG += libircclient-qt
    \endcode
 */

/*!
    \namespace Irc
    \brief The Irc namespace contains miscellaneous identifiers used throughout the LibIrcClient-Qt library.
 */

/*!
    Returns the LibIrcClient-Qt version number as string
    in form M.N.P (M = major, N = minor, P = patch).
 */

/*!
    Returns the version number of LibIrcClient-Qt at run-time as a string (for example, "1.2.3").
    This may be a different version than the version the application was compiled against.

    \sa IRC_VERSION_STR
 */
const char* Irc::version()
{
    return IRC_VERSION_STR;
}

/*!
    Returns the numeric RFC \a code as a string or \a 0 if the code is unknown.
 */
const char* Irc::Rfc::toString(uint code)
{
    int index = staticMetaObject.indexOfEnumerator("Numeric");
    Q_ASSERT(index != -1);
    QMetaEnum enumerator = staticMetaObject.enumerator(index);
    return enumerator.valueToKey(code);
}

/*!
    \class Irc::Rfc irc.h
    \brief The Irc::Rfc class enumerates command responses and error replies.

    The Irc::Rfc class enumerates command responses and error replies as defined
    in RFC 1459 (Internet Relay Chat Protocol - http://www.ietf.org/rfc/rfc1459.txt).

    \sa Irc::Rfc::Numeric
 */

/*!
    \enum Irc::Rfc::Numeric

    This enum describes the numeric message codes defined in the RFC.
 */

/*!
    \var Irc::Rfc::RPL_WELCOME
    \brief 001 Welcome to the Internet Relay Network \<nick\>!\<user\>\@\<host\>

    The server sends replies 001 to 004 to a user upon successful registration.
 */

/*!
    \var Irc::Rfc::RPL_YOURHOST
    \brief 002 Your host is \<servername\>, running version \<ver\>

    The server sends replies 001 to 004 to a user upon successful registration.
 */

/*!
    \var Irc::Rfc::RPL_CREATED
    \brief 003 This server was created \<date\>

    The server sends replies 001 to 004 to a user upon successful registration.
 */

/*!
    \var Irc::Rfc::RPL_MYINFO
    \brief 004 \<servername\> \<version\> \<available user modes\> \<available channel modes\>

    The server sends replies 001 to 004 to a user upon successful registration.
 */

/*!
    \var Irc::Rfc::RPL_BOUNCE
    \brief 005 Try server \<server name\>, port \<port number\>

    Sent by the server to a user to suggest an alternative server. This is often used when the connection is refused because the server is already full.
 */

/*!
    \var Irc::Rfc::RPL_USERHOST
    \brief 302 :*1\<reply\> *(

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ISON
    \brief 303 :*1\<nick\> *(

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_AWAY
    \brief 301 \<nick\> :\<away message\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_UNAWAY
    \brief 305 :You are no longer marked as being away

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_NOWAWAY
    \brief 306 :You have been marked as being away

    These replies are used with the AWAY command (if allowed). RPL_AWAY is sent to any client sending a PRIVMSG to a client which is away. RPL_AWAY is only sent by the server to which the client is connected. Replies RPL_UNAWAY and RPL_NOWAWAY are sent when the client removes and sets an AWAY message.
 */

/*!
    \var Irc::Rfc::RPL_WHOISUSER
    \brief 311 \<nick\> \<user\> \<host\> * :\<real name\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_WHOISSERVER
    \brief 312 \<nick\> \<server\> :\<server info\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_WHOISOPERATOR
    \brief 313 \<nick\> :is an IRC operator

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_WHOISIDLE
    \brief 317 \<nick\> \<integer\> :seconds idle

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFWHOIS
    \brief 318 \<nick\> :End of WHOIS list

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_WHOISCHANNELS
    \brief 319 "<nick> :*( ( "\@" / "+" ) \<channel\> " " )"

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_WHOWASUSER
    \brief 314 \<nick\> \<user\> \<host\> * :\<real name\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFWHOWAS
    \brief 369 \<nick\> :End of WHOWAS

    When replying to a WHOWAS message, a server MUST use the replies RPL_WHOWASUSER, RPL_WHOISSERVER or ERR_WASNOSUCHNICK for each nickname in the presented list. At the end of all reply batches, there MUST be RPL_ENDOFWHOWAS (even if there was only one reply and it was an error).
 */

/*!
    \var Irc::Rfc::RPL_LIST
    \brief 322 \<channel\> \<# visible\> :\<topic\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_LISTEND
    \brief 323 :End of LIST

    Replies RPL_LIST, RPL_LISTEND mark the actual replies with data and end of the server's response to a LIST command. If there are no channels available to return, only the end reply MUST be sent.
 */

/*!
    \var Irc::Rfc::RPL_UNIQOPIS
    \brief 325 \<channel\> \<nickname\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_CHANNELMODEIS
    \brief 324 \<channel\> \<mode\> \<mode params\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_CHANNELURL
    \brief 329 \<channel\> \<url\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_CHANNELCREATED
    \brief 329 \<channel\> \<datetime\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_NOTOPIC
    \brief 331 \<channel\> :No topic is set

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TOPIC
    \brief 332 \<channel\> :\<topic\>

    When sending a TOPIC message to determine the channel topic, one of two replies is sent. If the topic is set, RPL_TOPIC is sent back else RPL_NOTOPIC.
 */

/*!
    \var Irc::Rfc::RPL_TOPICSET
    \brief 333 \<channel\> \<user\> \<datetime\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_INVITING
    \brief 341 \<channel\> \<nick\>

    Returned by the server to indicate that the attempted INVITE message was successful and is being passed onto the end client.
 */

/*!
    \var Irc::Rfc::RPL_SUMMONING
    \brief 342 \<user\> :Summoning user to IRC

    Returned by a server answering a SUMMON message to indicate that it is summoning that user.
 */

/*!
    \var Irc::Rfc::RPL_INVITELIST
    \brief 346 \<channel\> \<invitemask\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFINVITELIST
    \brief 347 \<channel\> :End of channel invite list

    When listing the 'invitations masks' for a given channel, a server is required to send the list back using the RPL_INVITELIST and RPL_ENDOFINVITELIST messages. A separate RPL_INVITELIST is sent for each active mask. After the masks have been listed (or if none present) a RPL_ENDOFINVITELIST MUST be sent.
 */

/*!
    \var Irc::Rfc::RPL_EXCEPTLIST
    \brief 348 \<channel\> \<exceptionmask\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFEXCEPTLIST
    \brief 349 \<channel\> :End of channel exception list

    When listing the 'exception masks' for a given channel, a server is required to send the list back using the RPL_EXCEPTLIST and RPL_ENDOFEXCEPTLIST messages. A separate RPL_EXCEPTLIST is sent for each active mask. After the masks have been listed (or if none present) a RPL_ENDOFEXCEPTLIST MUST be sent.
 */

/*!
    \var Irc::Rfc::RPL_VERSION
    \brief 351 \<version\>.\<debuglevel\> \<server\> :\<comments\>

    Reply by the server showing its version details. The \<version\> is the version of the software being used (including any patchlevel revisions) and the \<debuglevel\> is used to indicate if the server is running in "debug mode". The "comments" field may contain any comments about the version or further version details.
 */

/*!
    \var Irc::Rfc::RPL_WHOREPLY
    \brief 352 \<channel\> \<user\> \<host\> \<server\> \<nick\> \<H|G\>[*][@|+] :\<hopcount\> \<real name\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFWHO
    \brief 315 \<name\> :End of WHO list

    The RPL_WHOREPLY and RPL_ENDOFWHO pair are used to answer a WHO message. The RPL_WHOREPLY is only sent if there is an appropriate match to the WHO query. If there is a list of parameters supplied with a WHO message, a RPL_ENDOFWHO MUST be sent after processing each list item with \<name\> being the item
 */

/*!
    \var Irc::Rfc::RPL_NAMREPLY
    \brief 353 \<channel\> :[[@|+]\<nick\> [[@|+]\<nick\> [...]]]

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFNAMES
    \brief 366 \<channel\> :End of NAMES list

    To reply to a NAMES message, a reply pair consisting of RPL_NAMREPLY and RPL_ENDOFNAMES is sent by the server back to the client. If there is no channel found as in the query, then only RPL_ENDOFNAMES is returned. The exception to this is when a NAMES message is sent with no parameters and all visible channels and contents are sent back in a series of RPL_NAMEREPLY messages with a RPL_ENDOFNAMES to mark the end.
 */

/*!
    \var Irc::Rfc::RPL_LINKS
    \brief 364 \<mask\> \<server\> :\<hopcount\> \<server info\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFLINKS
    \brief 365 \<mask\> :End of LINKS list

    In replying to the LINKS message, a server MUST send replies back using the RPL_LINKS numeric and mark the end of the list using an RPL_ENDOFLINKS reply.
 */

/*!
    \var Irc::Rfc::RPL_BANLIST
    \brief 367 \<channel\> \<banmask\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFBANLIST
    \brief 368 \<channel\> :End of channel ban list

    When listing the active 'bans' for a given channel, a server is required to send the list back using the RPL_BANLIST and RPL_ENDOFBANLIST messages. A separate RPL_BANLIST is sent for each active banmask. After the banmasks have been listed (or if none present) a RPL_ENDOFBANLIST MUST be sent.
 */

/*!
    \var Irc::Rfc::RPL_INFO
    \brief 371 :\<string\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFINFO
    \brief 374 :End of INFO list

    A server responding to an INFO message is required to send all its 'info' in a series of RPL_INFO messages with a RPL_ENDOFINFO reply to indicate the end of the replies.
 */

/*!
    \var Irc::Rfc::RPL_MOTDSTART
    \brief 375 :- \<server\> Message of the day -

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_MOTD
    \brief 372 :- \<text\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFMOTD
    \brief 376 :End of MOTD command

    When responding to the MOTD message and the MOTD file is found, the file is displayed line by line, with each line no longer than 80 characters, using RPL_MOTD format replies. These MUST be surrounded by a RPL_MOTDSTART (before the RPL_MOTDs) and an RPL_ENDOFMOTD (after).
 */

/*!
    \var Irc::Rfc::RPL_YOUREOPER
    \brief 381 :You are now an IRC operator

    RPL_YOUREOPER is sent back to a client which has just successfully issued an OPER message and gained operator status.
 */

/*!
    \var Irc::Rfc::RPL_REHASHING
    \brief 382 \<config file\> :Rehashing

    If the REHASH option is used and an operator sends a REHASH message, an RPL_REHASHING is sent back to the operator.
 */

/*!
    \var Irc::Rfc::RPL_YOURESERVICE
    \brief 383 You are service \<servicename\>

    Sent by the server to a service upon successful registration.
 */

/*!
    \var Irc::Rfc::RPL_TIME
    \brief 391 \<server\> :\<string showing server's local time\>

    When replying to the TIME message, a server MUST send the reply using the RPL_TIME format above. The string showing the time need only contain the correct day and time there. There is no further requirement for the time string.
 */

/*!
    \var Irc::Rfc::RPL_USERSSTART
    \brief 392 :UserID   Terminal  Host

    No description available in RFC
 */


/*!
    \var Irc::Rfc::RPL_USERS
    \brief 393 :\<username\> \<ttyline\> \<hostname\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ENDOFUSERS
    \brief 394 :End of users

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_NOUSERS
    \brief 395 :Nobody logged in

    If the USERS message is handled by a server, the replies RPL_USERSTART, RPL_USERS, RPL_ENDOFUSERS and RPL_NOUSERS are used. RPL_USERSSTART MUST be sent first, following by either a sequence of RPL_USERS or a single RPL_NOUSER. Following this is RPL_ENDOFUSERS.
 */


/*!
    \var Irc::Rfc::RPL_TRACELINK
    \brief 200 Link \<version \& debug level\> \<destination\>
                   \<next server\> V\<protocol version\>
                   \<link uptime in seconds\> \<backstream sendq\>
                   \<upstream sendq\>

    No description available in RFC
 */


/*!
    \var Irc::Rfc::RPL_TRACECONNECTING
    \brief 201 Try. \<class\> \<server\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACEHANDSHAKE
    \brief 202 H.S. \<class\> \<server\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACEUNKNOWN
    \brief 203 ???? \<class\> [\<client IP address in dot form\>]

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACEOPERATOR
    \brief 204 Oper \<class\> \<nick\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACEUSER
    \brief 205 User \<class\> \<nick\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACESERVER
    \brief 206 Serv \<class\> \<int\>S \<int\>C \<server\>
                   \<nick!user|*!*\>\@\<host|server\> V\<protocol version\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACESERVICE
    \brief 207 Service \<class\> \<name\> \<type\> \<active type\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACENEWTYPE
    \brief 208 \<newtype\> 0 \<client name\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACECLASS
    \brief 209 Class \<class\> \<count\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACELOG
    \brief 261 File \<logfile\> \<debug level\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_TRACEEND
    \brief 262 \<server name\> \<version \& debug level\> :End of TRACE

    The RPL_TRACE* are all returned by the server in response to the TRACE message. How many are returned is dependent on the TRACE message and whether it was sent by an operator or not. There is no predefined order for which occurs first. Replies RPL_TRACEUNKNOWN, RPL_TRACECONNECTING and RPL_TRACEHANDSHAKE are all used for connections which have not been fully established and are either unknown, still attempting to connect or in the process of completing the 'server handshake'. RPL_TRACELINK is sent by any server which handles a TRACE message and has to pass it on to another server. The list of RPL_TRACELINKs sent in response to a TRACE command traversing the IRC network should reflect the actual connectivity of the servers themselves along that path. RPL_TRACENEWTYPE is to be used for any connection which does not fit in the other categories but is being displayed anyway. RPL_TRACEEND is sent to indicate the end of the list.
 */

/*!
    \var Irc::Rfc::RPL_STATSLINKINFO
    \brief 211 \<linkname\> \<sendq\> \<sent messages\>
                   \<sent Kbytes\> \<received messages\>
                   \<received Kbytes\> \<time open\>

    reports statistics on a connection. \<linkname\> identifies the particular connection, \<sendq\> is the amount of data that is queued and waiting to be sent \<sent messages\> the number of messages sent, and \<sent Kbytes\> the amount of data sent, in Kbytes. \<received messages\> and \<received Kbytes\> are the equivalent of \<sent messages\> and \<sent Kbytes\> for received data, respectively. \<time open\> indicates how long ago the connection was opened, in seconds.
 */

/*!
    \var Irc::Rfc::RPL_STATSCOMMANDS
    \brief 212 \<command\> \<count\> \<byte count\> \<remote count\>

    reports statistics on commands usage.
 */

/*!
    \var Irc::Rfc::RPL_ENDOFSTATS
    \brief 219 \<stats letter\> :End of STATS report

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_STATSUPTIME
    \brief 242 :Server Up %d days %d:%02d:%02d

    reports the server uptime.
 */

/*!
    \var Irc::Rfc::RPL_STATSOLINE
    \brief 243 O \<hostmask\> * \<name\>

    reports the allowed hosts from where user may become IRC operators.
 */

/*!
    \var Irc::Rfc::RPL_UMODEIS
    \brief 221 \<user mode string\>

    To answer a query about a client's own mode, RPL_UMODEIS is sent back.
 */

/*!
    \var Irc::Rfc::RPL_SERVLIST
    \brief 234 \<name\> \<server\> \<mask\> \<type\> \<hopcount\> \<info\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_SERVLISTEND
    \brief 235 \<mask\> \<type\> :End of service listing

    When listing services in reply to a SERVLIST message, a server is required to send the list back using the RPL_SERVLIST and RPL_SERVLISTEND messages. A separate RPL_SERVLIST is sent for each service. After the services have been listed (or if none present) a RPL_SERVLISTEND MUST be sent.
 */

/*!
    \var Irc::Rfc::RPL_LUSERCLIENT
    \brief 251 :There are \<integer\> users and \<integer\>
                   services on \<integer\> servers

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_LUSEROP
    \brief 252 \<integer\> :operator(s) online

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_LUSERUNKNOWN
    \brief 253 \<integer\> :unknown connection(s)

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_LUSERCHANNELS
    \brief 254 \<integer\> :channels formed

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_LUSERME
    \brief 255 :I have \<integer\> clients and \<integer\>
                    servers

    In processing an LUSERS message, the server sends a set of replies from RPL_LUSERCLIENT, RPL_LUSEROP, RPL_USERUNKNOWN, RPL_LUSERCHANNELS and RPL_LUSERME. When replying, a server MUST send back RPL_LUSERCLIENT and RPL_LUSERME. The other replies are only sent back if a non-zero count is found for them.
 */

/*!
    \var Irc::Rfc::RPL_ADMINME
    \brief 256 \<server\> :Administrative info

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ADMINLOC1
    \brief 257 :\<admin info\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ADMINLOC2
    \brief 258 :\<admin info\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::RPL_ADMINEMAIL
    \brief 259 :\<admin info\>

    When replying to an ADMIN message, a server is expected to use replies RPL_ADMINME through to RPL_ADMINEMAIL and provide a text message with each. For RPL_ADMINLOC1 a description of what city, state and country the server is in is expected, followed by details of the institution (RPL_ADMINLOC2) and finally the administrative contact for the server (an email address here is REQUIRED) in RPL_ADMINEMAIL.
 */

/*!
    \var Irc::Rfc::RPL_TRYAGAIN
    \brief 263 \<command\> :Please wait a while and try again.

    When a server drops a command without processing it, it MUST use the reply RPL_TRYAGAIN to inform the originating client.
 */

/*!
    \var Irc::Rfc::ERR_NOSUCHNICK
    \brief 401 \<nickname\> :No such nick/channel

    Used to indicate the nickname parameter supplied to a command is currently unused.
 */

/*!
    \var Irc::Rfc::ERR_NOSUCHSERVER
    \brief 402 \<server name\> :No such server

    Used to indicate the server name given currently does not exist.
 */

/*!
    \var Irc::Rfc::ERR_NOSUCHCHANNEL
    \brief 403 \<channel name\> :No such channel

    Used to indicate the given channel name is invalid.
 */

/*!
    \var Irc::Rfc::ERR_CANNOTSENDTOCHAN
    \brief 404 \<channel name\> :Cannot send to channel

    Sent to a user who is either (a) not on a channel which is mode +n or (b) not a chanop (or mode +v) on a channel which has mode +m set or where the user is banned and is trying to send a PRIVMSG message to that channel.
 */

/*!
    \var Irc::Rfc::ERR_TOOMANYCHANNELS
    \brief 405 \<channel name\> :You have joined too many channels

    Sent to a user when they have joined the maximum number of allowed channels and they try to join another channel.
 */

/*!
    \var Irc::Rfc::ERR_WASNOSUCHNICK
    \brief 406 \<nickname\> :There was no such nickname

    Returned by WHOWAS to indicate there is no history information for that nickname.
 */

/*!
    \var Irc::Rfc::ERR_TOOMANYTARGETS
    \brief 407 \<target\> :\<error code\> recipients. \<abort message\>

    Returned to a client which is attempting to send a PRIVMSG/NOTICE using the user\@host destination format and for a user\@host which has several occurrences. - Returned to a client which trying to send a PRIVMSG/NOTICE to too many recipients. - Returned to a client which is attempting to JOIN a safe channel using the shortname when there are more than one such channel.
 */

/*!
    \var Irc::Rfc::ERR_NOSUCHSERVICE
    \brief 408 \<service name\> :No such service

    Returned to a client which is attempting to send a SQUERY to a service which does not exist.
 */

/*!
    \var Irc::Rfc::ERR_NOORIGIN
    \brief 409 :No origin specified

    PING or PONG message missing the originator parameter.
 */

/*!
    \var Irc::Rfc::ERR_NORECIPIENT
    \brief 411 :No recipient given (\<command\>)

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_NOTEXTTOSEND
    \brief 412 :No text to send

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_NOTOPLEVEL
    \brief 413 \<mask\> :No toplevel domain specified

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_WILDTOPLEVEL
    \brief 414 \<mask\> :Wildcard in toplevel domain

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_BADMASK
    \brief 415 \<mask\> :Bad Server/host mask

    412 - 415 are returned by PRIVMSG to indicate that the message wasn't delivered for some reason. ERR_NOTOPLEVEL and ERR_WILDTOPLEVEL are errors that are returned when an invalid use of "PRIVMSG $\<server\>" or "PRIVMSG #\<host\>" is attempted.
 */

/*!
    \var Irc::Rfc::ERR_UNKNOWNCOMMAND
    \brief 421 \<command\> :Unknown command

    Returned to a registered client to indicate that the command sent is unknown by the server.
 */

/*!
    \var Irc::Rfc::ERR_NOMOTD
    \brief 422 :MOTD File is missing

    Server's MOTD file could not be opened by the server.
 */

/*!
    \var Irc::Rfc::ERR_NOADMININFO
    \brief 423 \<server\> :No administrative info available

    Returned by a server in response to an ADMIN message when there is an error in finding the appropriate information.
 */

/*!
    \var Irc::Rfc::ERR_FILEERROR
    \brief 424 :File error doing \<file op\> on \<file\>

    Generic error message used to report a failed file operation during the processing of a message.
 */

/*!
    \var Irc::Rfc::ERR_NONICKNAMEGIVEN
    \brief 431 :No nickname given

    Returned when a nickname parameter expected for a command and isn't found.
 */

/*!
    \var Irc::Rfc::ERR_ERRONEUSNICKNAME
    \brief 432 \<nick\> :Erroneous nickname

    Returned after receiving a NICK message which contains characters which do not fall in the defined set. See section 2.3.1 for details on valid nicknames.
 */

/*!
    \var Irc::Rfc::ERR_NICKNAMEINUSE
    \brief 433 \<nick\> :Nickname is already in use

    Returned when a NICK message is processed that results in an attempt to change to a currently existing nickname.
 */

/*!
    \var Irc::Rfc::ERR_NICKCOLLISION
    \brief 436 \<nick\> :Nickname collision KILL from \<user\>\@\<host\>

    Returned by a server to a client when it detects a nickname collision (registered of a NICK that already exists by another server).
 */

/*!
    \var Irc::Rfc::ERR_UNAVAILRESOURCE
    \brief 437 \<nick/channel\> :Nick/channel is temporarily unavailable

    Returned by a server to a user trying to join a channel currently blocked by the channel delay mechanism. - Returned by a server to a user trying to change nickname when the desired nickname is blocked by the nick delay mechanism.
 */

/*!
    \var Irc::Rfc::ERR_USERNOTINCHANNEL
    \brief 441 \<nick\> \<channel\> :They aren't on that channel

    Returned by the server to indicate that the target user of the command is not on the given channel.
 */

/*!
    \var Irc::Rfc::ERR_NOTONCHANNEL
    \brief 442 \<channel\> :You're not on that channel

    Returned by the server whenever a client tries to perform a channel affecting command for which the client isn't a member.
 */

/*!
    \var Irc::Rfc::ERR_USERONCHANNEL
    \brief 443 \<user\> \<channel\> :is already on channel

    Returned when a client tries to invite a user to a channel they are already on.
 */

/*!
    \var Irc::Rfc::ERR_NOLOGIN
    \brief 444 \<user\> :User not logged in

    Returned by the summon after a SUMMON command for a user was unable to be performed since they were not logged in.
 */

/*!
    \var Irc::Rfc::ERR_SUMMONDISABLED
    \brief 445 :SUMMON has been disabled

    Returned as a response to the SUMMON command. MUST be returned by any server which doesn't implement it.
 */

/*!
    \var Irc::Rfc::ERR_USERSDISABLED
    \brief 446 :USERS has been disabled

    Returned as a response to the USERS command. MUST be returned by any server which does not implement it.
 */

/*!
    \var Irc::Rfc::ERR_NOTREGISTERED
    \brief 451 :You have not registered

    Returned by the server to indicate that the client MUST be registered before the server will allow it to be parsed in detail.
 */

/*!
    \var Irc::Rfc::ERR_NEEDMOREPARAMS
    \brief 461 \<command\> :Not enough parameters

    Returned by the server by numerous commands to indicate to the client that it didn't supply enough parameters.
 */

/*!
    \var Irc::Rfc::ERR_ALREADYREGISTRED
    \brief 462 :Unauthorized command (already registered)

    Returned by the server to any link which tries to change part of the registered details (such as password or user details from second USER message).
 */

/*!
    \var Irc::Rfc::ERR_NOPERMFORHOST
    \brief 463 :Your host isn't among the privileged

    Returned to a client which attempts to register with a server which does not been setup to allow connections from the host the attempted connection is tried.
 */

/*!
    \var Irc::Rfc::ERR_PASSWDMISMATCH
    \brief 464 :Password incorrect

    Returned to indicate a failed attempt at registering a connection for which a password was required and was either not given or incorrect.
 */

/*!
    \var Irc::Rfc::ERR_YOUREBANNEDCREEP
    \brief 465 :You are banned from this server

    Returned after an attempt to connect and register yourself with a server which has been setup to explicitly deny connections to you.
 */

/*!
    \var Irc::Rfc::ERR_YOUWILLBEBANNED
    \brief 466 :You will be banned from this server

    Sent by a server to a user to inform that access to the server will soon be denied.
 */

/*!
    \var Irc::Rfc::ERR_KEYSET
    \brief 467 \<channel\> :Channel key already set

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_CHANNELISFULL
    \brief 471 \<channel\> :Cannot join channel (+l)

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_UNKNOWNMODE
    \brief 472 \<char\> :is unknown mode char to me for \<channel\>

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_INVITEONLYCHAN
    \brief 473 \<channel\> :Cannot join channel (+i)

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_BANNEDFROMCHAN
    \brief 474 \<channel\> :Cannot join channel (+b)

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_BADCHANNELKEY
    \brief 475 \<channel\> :Cannot join channel (+k)

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_BADCHANMASK
    \brief 476 \<channel\> :Bad Channel Mask

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_NOCHANMODES
    \brief 477 \<channel\> :Channel doesn't support modes

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_BANLISTFULL
    \brief 478 \<channel\> \<char\> :Channel list is full

    No description available in RFC
 */

/*!
    \var Irc::Rfc::ERR_NOPRIVILEGES
    \brief 481 :Permission Denied- You're not an IRC operator

    Any command requiring operator privileges to operate MUST return this error to indicate the attempt was unsuccessful.
 */

/*!
    \var Irc::Rfc::ERR_CHANOPRIVSNEEDED
    \brief 482 \<channel\> :You're not channel operator

    Any command requiring 'chanop' privileges (such as MODE messages) MUST return this error if the client making the attempt is not a chanop on the specified channel.
 */

/*!
    \var Irc::Rfc::ERR_CANTKILLSERVER
    \brief 483 :You can't kill a server!

    Any attempts to use the KILL command on a server are to be refused and this error returned directly to the client.
 */

/*!
    \var Irc::Rfc::ERR_RESTRICTED
    \brief 484 :Your connection is restricted!

    Sent by the server to a user upon connection to indicate the restricted nature of the connection (user mode "+r").
 */

/*!
    \var Irc::Rfc::ERR_UNIQOPPRIVSNEEDED
    \brief 485 :You're not the original channel operator

    Any MODE requiring "channel creator" privileges MUST return this error if the client making the attempt is not a chanop on the specified channel.
 */

/*!
    \var Irc::Rfc::ERR_NOOPERHOST
    \brief 491 :No O-lines for your host

    If a client sends an OPER message and the server has not been configured to allow connections from the client's host as an operator, this error MUST be returned.
 */

/*!
    \var Irc::Rfc::ERR_UMODEUNKNOWNFLAG
    \brief 501 :Unknown MODE flag

    Returned by the server to indicate that a MODE message was sent with a nickname parameter and that the a mode flag sent was not recognized.
 */

/*!
    \var Irc::Rfc::ERR_USERSDONTMATCH
    \brief 502 :Cannot change mode for other users

    Error sent to any user trying to view or change the user mode for a user other than themselves.
 */
