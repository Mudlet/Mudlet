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

#include "irc.h"
#include <QMetaEnum>

/*! \mainpage Communi - a cross-platform IRC client library written with Qt 4

    \section introduction Introduction

    Communi, formerly known as LibIrcClient-Qt, is a cross-platform IRC
    client library written with Qt 4. IRC (Internet Relay Chat protocol)
    is a simple text-based communication protocol.

    \section install Installation

    To compile Communi, do the standard mantra:
    \code
    $ qmake
    $ make
    $ sudo make install
    \endcode

    The default build config is resolved by qmake. To build Communi
    specifically in release or debug mode, you may pass additional
    "-config release" or "-config debug" parameters to qmake, respectively.
    Furthermore, in order to build a static version of Communi, you
    may pass "-config static".

    \section usage Usage

    Add the following line to your qmake project (.pro) file:
    \code
    CONFIG += communi
    \endcode

    This adds the necessary include paths and linker rules in order to use the library.

    Communi in a nutshell:
    \li IrcSession manages the connection to an IRC server
    \li IrcMessage represents a message received from an IRC server via IrcSession::messageReceived().
    \li IrcCommand represents a command sent to an IRC server via IrcSession::sendCommand().

    \defgroup core Core classes
    \brief The list of core classes to get started with.

    \defgroup message Message classes
    \brief The list of available IRC message classes.

    \defgroup utility Utility classes
    \brief The list of utility classes.
 */

/*!
    \file irc.h
    \brief #include &lt;Irc&gt;
 */

/*!
    \class Irc irc.h <Irc>
    \ingroup utility
    \brief The Irc class contains miscellaneous identifiers used throughout the library.
 */

/*!
    Returns the version number of Communi at run-time as a string (for example, "1.2.3").
    This may be a different version than the version the application was compiled against.

    \sa COMMUNI_VERSION and COMMUNI_VERSION_STR
 */
const char* Irc::version()
{
    return COMMUNI_VERSION_STR;
}

/*!
    Returns the numeric \a code as a string or \a 0 if the code is unknown.

    \sa Irc::Code and IrcNumericMessage::code()
 */
const char* Irc::toString(int code)
{
    int index = staticMetaObject.indexOfEnumerator("Code");
    Q_ASSERT(index != -1);
    QMetaEnum enumerator = staticMetaObject.enumerator(index);
    return enumerator.valueToKey(code);
}

/*!
    \enum Irc::Code

    The command responses and error replies as defined in
    <a href="http://tools.ietf.org/html/rfc1459">RFC 1459</a>,
    <a href="http://tools.ietf.org/html/rfc2812">RFC 2812</a>,
    and various <a href="http://www.alien.net.au/irc/irc2numerics.html">IRCd specific extensions</a>.
 */

/*!
    \var Irc::RPL_WELCOME
    \brief 1
 */
/*!
    \var Irc::RPL_YOURHOST
    \brief 2
 */
/*!
    \var Irc::RPL_CREATED
    \brief 3
 */
/*!
    \var Irc::RPL_MYINFO
    \brief 4
 */
/*!
    \var Irc::RPL_ISUPPORT
    \brief 5
 */
/*!
    \var Irc::RPL_SNOMASK
    \brief 8
 */
/*!
    \var Irc::RPL_STATMEMTOT
    \brief 9
 */
/*!
    \var Irc::RPL_BOUNCE
    \brief 10
 */
/*!
    \var Irc::RPL_STATMEM
    \brief 10
 */
/*!
    \var Irc::RPL_YOURCOOKIE
    \brief 14
 */
/*!
    \var Irc::RPL_YOURID
    \brief 42
 */
/*!
    \var Irc::RPL_SAVENICK
    \brief 43
 */
/*!
    \var Irc::RPL_ATTEMPTINGJUNC
    \brief 50
 */
/*!
    \var Irc::RPL_ATTEMPTINGREROUTE
    \brief 51
 */
/*!
    \var Irc::RPL_TRACELINK
    \brief 200
 */
/*!
    \var Irc::RPL_TRACECONNECTING
    \brief 201
 */
/*!
    \var Irc::RPL_TRACEHANDSHAKE
    \brief 202
 */
/*!
    \var Irc::RPL_TRACEUNKNOWN
    \brief 203
 */
/*!
    \var Irc::RPL_TRACEOPERATOR
    \brief 204
 */
/*!
    \var Irc::RPL_TRACEUSER
    \brief 205
 */
/*!
    \var Irc::RPL_TRACESERVER
    \brief 206
 */
/*!
    \var Irc::RPL_TRACESERVICE
    \brief 207
 */
/*!
    \var Irc::RPL_TRACENEWTYPE
    \brief 208
 */
/*!
    \var Irc::RPL_TRACECLASS
    \brief 209
 */
/*!
    \var Irc::RPL_TRACERECONNECT
    \brief 210
 */
/*!
    \var Irc::RPL_STATS
    \brief 210
 */
/*!
    \var Irc::RPL_STATSLINKINFO
    \brief 211
 */
/*!
    \var Irc::RPL_STATSCOMMANDS
    \brief 212
 */
/*!
    \var Irc::RPL_STATSCLINE
    \brief 213
 */
/*!
    \var Irc::RPL_STATSNLINE
    \brief 214
 */
/*!
    \var Irc::RPL_STATSILINE
    \brief 215
 */
/*!
    \var Irc::RPL_STATSKLINE
    \brief 216
 */
/*!
    \var Irc::RPL_STATSQLINE
    \brief 217
 */
/*!
    \var Irc::RPL_STATSYLINE
    \brief 218
 */
/*!
    \var Irc::RPL_ENDOFSTATS
    \brief 219
 */
/*!
    \var Irc::RPL_UMODEIS
    \brief 221
 */
/*!
    \var Irc::RPL_MODLIST
    \brief 222
 */
/*!
    \var Irc::RPL_SQLINE_NICK
    \brief 222
 */
/*!
    \var Irc::RPL_STATSZLINE
    \brief 225
 */
/*!
    \var Irc::RPL_STATSCOUNT
    \brief 226
 */
/*!
    \var Irc::RPL_SERVICEINFO
    \brief 231
 */
/*!
    \var Irc::RPL_ENDOFSERVICES
    \brief 232
 */
/*!
    \var Irc::RPL_SERVICE
    \brief 233
 */
/*!
    \var Irc::RPL_SERVLIST
    \brief 234
 */
/*!
    \var Irc::RPL_SERVLISTEND
    \brief 235
 */
/*!
    \var Irc::RPL_STATSVERBOSE
    \brief 236
 */
/*!
    \var Irc::RPL_STATSENGINE
    \brief 237
 */
/*!
    \var Irc::RPL_STATSIAUTH
    \brief 239
 */
/*!
    \var Irc::RPL_STATSVLINE
    \brief 240
 */
/*!
    \var Irc::RPL_STATSLLINE
    \brief 241
 */
/*!
    \var Irc::RPL_STATSUPTIME
    \brief 242
 */
/*!
    \var Irc::RPL_STATSOLINE
    \brief 243
 */
/*!
    \var Irc::RPL_STATSHLINE
    \brief 244
 */
/*!
    \var Irc::RPL_STATSSLINE
    \brief 245
 */
/*!
    \var Irc::RPL_STATSPING
    \brief 246
 */
/*!
    \var Irc::RPL_STATSBLINE
    \brief 247
 */
/*!
    \var Irc::RPL_STATSDEFINE
    \brief 248
 */
/*!
    \var Irc::RPL_STATSDEBUG
    \brief 249
 */
/*!
    \var Irc::RPL_STATSDLINE
    \brief 250
 */
/*!
    \var Irc::RPL_STATSCONN
    \brief 250
 */
/*!
    \var Irc::RPL_LUSERCLIENT
    \brief 251
 */
/*!
    \var Irc::RPL_LUSEROP
    \brief 252
 */
/*!
    \var Irc::RPL_LUSERUNKNOWN
    \brief 253
 */
/*!
    \var Irc::RPL_LUSERCHANNELS
    \brief 254
 */
/*!
    \var Irc::RPL_LUSERME
    \brief 255
 */
/*!
    \var Irc::RPL_ADMINME
    \brief 256
 */
/*!
    \var Irc::RPL_ADMINLOC1
    \brief 257
 */
/*!
    \var Irc::RPL_ADMINLOC2
    \brief 258
 */
/*!
    \var Irc::RPL_ADMINEMAIL
    \brief 259
 */
/*!
    \var Irc::RPL_TRACELOG
    \brief 261
 */
/*!
    \var Irc::RPL_TRACEPING
    \brief 262
 */
/*!
    \var Irc::RPL_TRACEEND
    \brief 262
 */
/*!
    \var Irc::RPL_TRYAGAIN
    \brief 263
 */
/*!
    \var Irc::RPL_LOCALUSERS
    \brief 265
 */
/*!
    \var Irc::RPL_GLOBALUSERS
    \brief 266
 */
/*!
    \var Irc::RPL_START_NETSTAT
    \brief 267
 */
/*!
    \var Irc::RPL_NETSTAT
    \brief 268
 */
/*!
    \var Irc::RPL_END_NETSTAT
    \brief 269
 */
/*!
    \var Irc::RPL_PRIVS
    \brief 270
 */
/*!
    \var Irc::RPL_SILELIST
    \brief 271
 */
/*!
    \var Irc::RPL_ENDOFSILELIST
    \brief 272
 */
/*!
    \var Irc::RPL_NOTIFY
    \brief 273
 */
/*!
    \var Irc::RPL_ENDNOTIFY
    \brief 274
 */
/*!
    \var Irc::RPL_STATSDELTA
    \brief 274
 */
/*!
    \var Irc::RPL_VCHANEXIST
    \brief 276
 */
/*!
    \var Irc::RPL_VCHANLIST
    \brief 277
 */
/*!
    \var Irc::RPL_VCHANHELP
    \brief 278
 */
/*!
    \var Irc::RPL_GLIST
    \brief 280
 */
/*!
    \var Irc::RPL_ENDOFGLIST
    \brief 281
 */
/*!
    \var Irc::RPL_ACCEPTLIST
    \brief 281
 */
/*!
    \var Irc::RPL_ENDOFACCEPT
    \brief 282
 */
/*!
    \var Irc::RPL_JUPELIST
    \brief 282
 */
/*!
    \var Irc::RPL_ENDOFJUPELIST
    \brief 283
 */
/*!
    \var Irc::RPL_FEATURE
    \brief 284
 */
/*!
    \var Irc::RPL_GLIST_HASH
    \brief 285
 */
/*!
    \var Irc::RPL_CHANINFO_HANDLE
    \brief 285
 */
/*!
    \var Irc::RPL_NEWHOSTIS
    \brief 285
 */
/*!
    \var Irc::RPL_CHANINFO_USERS
    \brief 286
 */
/*!
    \var Irc::RPL_CHKHEAD
    \brief 286
 */
/*!
    \var Irc::RPL_CHANINFO_CHOPS
    \brief 287
 */
/*!
    \var Irc::RPL_CHANUSER
    \brief 287
 */
/*!
    \var Irc::RPL_CHANINFO_VOICES
    \brief 288
 */
/*!
    \var Irc::RPL_PATCHHEAD
    \brief 288
 */
/*!
    \var Irc::RPL_CHANINFO_AWAY
    \brief 289
 */
/*!
    \var Irc::RPL_PATCHCON
    \brief 289
 */
/*!
    \var Irc::RPL_CHANINFO_OPERS
    \brief 290
 */
/*!
    \var Irc::RPL_HELPHDR
    \brief 290
 */
/*!
    \var Irc::RPL_DATASTR
    \brief 290
 */
/*!
    \var Irc::RPL_CHANINFO_BANNED
    \brief 291
 */
/*!
    \var Irc::RPL_HELPOP
    \brief 291
 */
/*!
    \var Irc::RPL_ENDOFCHECK
    \brief 291
 */
/*!
    \var Irc::RPL_CHANINFO_BANS
    \brief 292
 */
/*!
    \var Irc::RPL_HELPTLR
    \brief 292
 */
/*!
    \var Irc::RPL_CHANINFO_INVITE
    \brief 293
 */
/*!
    \var Irc::RPL_HELPHLP
    \brief 293
 */
/*!
    \var Irc::RPL_CHANINFO_INVITES
    \brief 294
 */
/*!
    \var Irc::RPL_HELPFWD
    \brief 294
 */
/*!
    \var Irc::RPL_CHANINFO_KICK
    \brief 295
 */
/*!
    \var Irc::RPL_HELPIGN
    \brief 295
 */
/*!
    \var Irc::RPL_CHANINFO_KICKS
    \brief 296
 */
/*!
    \var Irc::RPL_END_CHANINFO
    \brief 299
 */
/*!
    \var Irc::RPL_NONE
    \brief 300
 */
/*!
    \var Irc::RPL_AWAY
    \brief 301
 */
/*!
    \var Irc::RPL_USERHOST
    \brief 302
 */
/*!
    \var Irc::RPL_ISON
    \brief 303
 */
/*!
    \var Irc::RPL_TEXT
    \brief 304
 */
/*!
    \var Irc::RPL_UNAWAY
    \brief 305
 */
/*!
    \var Irc::RPL_NOWAWAY
    \brief 306
 */
/*!
    \var Irc::RPL_WHOISREGNICK
    \brief 307
 */
/*!
    \var Irc::RPL_SUSERHOST
    \brief 307
 */
/*!
    \var Irc::RPL_NOTIFYACTION
    \brief 308
 */
/*!
    \var Irc::RPL_WHOISADMIN
    \brief 308
 */
/*!
    \var Irc::RPL_NICKTRACE
    \brief 309
 */
/*!
    \var Irc::RPL_WHOISSADMIN
    \brief 309
 */
/*!
    \var Irc::RPL_WHOISHELPER
    \brief 309
 */
/*!
    \var Irc::RPL_WHOISSVCMSG
    \brief 310
 */
/*!
    \var Irc::RPL_WHOISHELPOP
    \brief 310
 */
/*!
    \var Irc::RPL_WHOISSERVICE
    \brief 310
 */
/*!
    \var Irc::RPL_WHOISUSER
    \brief 311
 */
/*!
    \var Irc::RPL_WHOISSERVER
    \brief 312
 */
/*!
    \var Irc::RPL_WHOISOPERATOR
    \brief 313
 */
/*!
    \var Irc::RPL_WHOWASUSER
    \brief 314
 */
/*!
    \var Irc::RPL_ENDOFWHO
    \brief 315
 */
/*!
    \var Irc::RPL_WHOISCHANOP
    \brief 316
 */
/*!
    \var Irc::RPL_WHOISIDLE
    \brief 317
 */
/*!
    \var Irc::RPL_ENDOFWHOIS
    \brief 318
 */
/*!
    \var Irc::RPL_WHOISCHANNELS
    \brief 319
 */
/*!
    \var Irc::RPL_WHOISVIRT
    \brief 320
 */
/*!
    \var Irc::RPL_WHOIS_HIDDEN
    \brief 320
 */
/*!
    \var Irc::RPL_WHOISSPECIAL
    \brief 320
 */
/*!
    \var Irc::RPL_LISTSTART
    \brief 321
 */
/*!
    \var Irc::RPL_LIST
    \brief 322
 */
/*!
    \var Irc::RPL_LISTEND
    \brief 323
 */
/*!
    \var Irc::RPL_CHANNELMODEIS
    \brief 324
 */
/*!
    \var Irc::RPL_UNIQOPIS
    \brief 325
 */
/*!
    \var Irc::RPL_CHANNELPASSIS
    \brief 325
 */
/*!
    \var Irc::RPL_NOCHANPASS
    \brief 326
 */
/*!
    \var Irc::RPL_CHPASSUNKNOWN
    \brief 327
 */
/*!
    \var Irc::RPL_CHANNEL_URL
    \brief 328
 */
/*!
    \var Irc::RPL_CREATIONTIME
    \brief 329
 */
/*!
    \var Irc::RPL_WHOWAS_TIME
    \brief 330
 */
/*!
    \var Irc::RPL_WHOISACCOUNT
    \brief 330
 */
/*!
    \var Irc::RPL_NOTOPIC
    \brief 331
 */
/*!
    \var Irc::RPL_TOPIC
    \brief 332
 */
/*!
    \var Irc::RPL_TOPICWHOTIME
    \brief 333
 */
/*!
    \var Irc::RPL_LISTUSAGE
    \brief 334
 */
/*!
    \var Irc::RPL_COMMANDSYNTAX
    \brief 334
 */
/*!
    \var Irc::RPL_LISTSYNTAX
    \brief 334
 */
/*!
    \var Irc::RPL_CHANPASSOK
    \brief 338
 */
/*!
    \var Irc::RPL_WHOISACTUALLY
    \brief 338
 */
/*!
    \var Irc::RPL_BADCHANPASS
    \brief 339
 */
/*!
    \var Irc::RPL_INVITING
    \brief 341
 */
/*!
    \var Irc::RPL_SUMMONING
    \brief 342
 */
/*!
    \var Irc::RPL_INVITED
    \brief 345
 */
/*!
    \var Irc::RPL_INVITELIST
    \brief 346
 */
/*!
    \var Irc::RPL_ENDOFINVITELIST
    \brief 347
 */
/*!
    \var Irc::RPL_EXCEPTLIST
    \brief 348
 */
/*!
    \var Irc::RPL_ENDOFEXCEPTLIST
    \brief 349
 */
/*!
    \var Irc::RPL_VERSION
    \brief 351
 */
/*!
    \var Irc::RPL_WHOREPLY
    \brief 352
 */
/*!
    \var Irc::RPL_NAMREPLY
    \brief 353
 */
/*!
    \var Irc::RPL_WHOSPCRPL
    \brief 354
 */
/*!
    \var Irc::RPL_NAMREPLY_
    \brief 355
 */
/*!
    \var Irc::RPL_KILLDONE
    \brief 361
 */
/*!
    \var Irc::RPL_CLOSING
    \brief 362
 */
/*!
    \var Irc::RPL_CLOSEEND
    \brief 363
 */
/*!
    \var Irc::RPL_LINKS
    \brief 364
 */
/*!
    \var Irc::RPL_ENDOFLINKS
    \brief 365
 */
/*!
    \var Irc::RPL_ENDOFNAMES
    \brief 366
 */
/*!
    \var Irc::RPL_BANLIST
    \brief 367
 */
/*!
    \var Irc::RPL_ENDOFBANLIST
    \brief 368
 */
/*!
    \var Irc::RPL_ENDOFWHOWAS
    \brief 369
 */
/*!
    \var Irc::RPL_INFO
    \brief 371
 */
/*!
    \var Irc::RPL_MOTD
    \brief 372
 */
/*!
    \var Irc::RPL_INFOSTART
    \brief 373
 */
/*!
    \var Irc::RPL_ENDOFINFO
    \brief 374
 */
/*!
    \var Irc::RPL_MOTDSTART
    \brief 375
 */
/*!
    \var Irc::RPL_ENDOFMOTD
    \brief 376
 */
/*!
    \var Irc::RPL_KICKEXPIRED
    \brief 377
 */
/*!
    \var Irc::RPL_SPAM
    \brief 377
 */
/*!
    \var Irc::RPL_BANEXPIRED
    \brief 378
 */
/*!
    \var Irc::RPL_WHOISHOST
    \brief 378
 */
/*!
    \var Irc::RPL_KICKLINKED
    \brief 379
 */
/*!
    \var Irc::RPL_WHOISMODES
    \brief 379
 */
/*!
    \var Irc::RPL_BANLINKED
    \brief 380
 */
/*!
    \var Irc::RPL_YOURHELPER
    \brief 380
 */
/*!
    \var Irc::RPL_YOUREOPER
    \brief 381
 */
/*!
    \var Irc::RPL_REHASHING
    \brief 382
 */
/*!
    \var Irc::RPL_YOURESERVICE
    \brief 383
 */
/*!
    \var Irc::RPL_MYPORTIS
    \brief 384
 */
/*!
    \var Irc::RPL_NOTOPERANYMORE
    \brief 385
 */
/*!
    \var Irc::RPL_QLIST
    \brief 386
 */
/*!
    \var Irc::RPL_IRCOPS
    \brief 386
 */
/*!
    \var Irc::RPL_ENDOFQLIST
    \brief 387
 */
/*!
    \var Irc::RPL_ENDOFIRCOPS
    \brief 387
 */
/*!
    \var Irc::RPL_ALIST
    \brief 388
 */
/*!
    \var Irc::RPL_ENDOFALIST
    \brief 389
 */
/*!
    \var Irc::RPL_TIME
    \brief 391
 */
/*!
    \var Irc::RPL_USERSSTART
    \brief 392
 */
/*!
    \var Irc::RPL_USERS
    \brief 393
 */
/*!
    \var Irc::RPL_ENDOFUSERS
    \brief 394
 */
/*!
    \var Irc::RPL_NOUSERS
    \brief 395
 */
/*!
    \var Irc::RPL_HOSTHIDDEN
    \brief 396
 */
/*!
    \var Irc::ERR_UNKNOWNERROR
    \brief 400
 */
/*!
    \var Irc::ERR_NOSUCHNICK
    \brief 401
 */
/*!
    \var Irc::ERR_NOSUCHSERVER
    \brief 402
 */
/*!
    \var Irc::ERR_NOSUCHCHANNEL
    \brief 403
 */
/*!
    \var Irc::ERR_CANNOTSENDTOCHAN
    \brief 404
 */
/*!
    \var Irc::ERR_TOOMANYCHANNELS
    \brief 405
 */
/*!
    \var Irc::ERR_WASNOSUCHNICK
    \brief 406
 */
/*!
    \var Irc::ERR_TOOMANYTARGETS
    \brief 407
 */
/*!
    \var Irc::ERR_NOSUCHSERVICE
    \brief 408
 */
/*!
    \var Irc::ERR_NOCOLORSONCHAN
    \brief 408
 */
/*!
    \var Irc::ERR_NOORIGIN
    \brief 409
 */
/*!
    \var Irc::ERR_NORECIPIENT
    \brief 411
 */
/*!
    \var Irc::ERR_NOTEXTTOSEND
    \brief 412
 */
/*!
    \var Irc::ERR_NOTOPLEVEL
    \brief 413
 */
/*!
    \var Irc::ERR_WILDTOPLEVEL
    \brief 414
 */
/*!
    \var Irc::ERR_BADMASK
    \brief 415
 */
/*!
    \var Irc::ERR_TOOMANYMATCHES
    \brief 416
 */
/*!
    \var Irc::ERR_QUERYTOOLONG
    \brief 416
 */
/*!
    \var Irc::ERR_LENGTHTRUNCATED
    \brief 419
 */
/*!
    \var Irc::ERR_UNKNOWNCOMMAND
    \brief 421
 */
/*!
    \var Irc::ERR_NOMOTD
    \brief 422
 */
/*!
    \var Irc::ERR_NOADMININFO
    \brief 423
 */
/*!
    \var Irc::ERR_FILEERROR
    \brief 424
 */
/*!
    \var Irc::ERR_NOOPERMOTD
    \brief 425
 */
/*!
    \var Irc::ERR_TOOMANYAWAY
    \brief 429
 */
/*!
    \var Irc::ERR_EVENTNICKCHANGE
    \brief 430
 */
/*!
    \var Irc::ERR_NONICKNAMEGIVEN
    \brief 431
 */
/*!
    \var Irc::ERR_ERRONEUSNICKNAME
    \brief 432
 */
/*!
    \var Irc::ERR_NICKNAMEINUSE
    \brief 433
 */
/*!
    \var Irc::ERR_SERVICENAMEINUSE
    \brief 434
 */
/*!
    \var Irc::ERR_NORULES
    \brief 434
 */
/*!
    \var Irc::ERR_SERVICECONFUSED
    \brief 435
 */
/*!
    \var Irc::ERR_BANONCHAN
    \brief 435
 */
/*!
    \var Irc::ERR_NICKCOLLISION
    \brief 436
 */
/*!
    \var Irc::ERR_UNAVAILRESOURCE
    \brief 437
 */
/*!
    \var Irc::ERR_BANNICKCHANGE
    \brief 437
 */
/*!
    \var Irc::ERR_NICKTOOFAST
    \brief 438
 */
/*!
    \var Irc::ERR_DEAD
    \brief 438
 */
/*!
    \var Irc::ERR_TARGETTOOFAST
    \brief 439
 */
/*!
    \var Irc::ERR_SERVICESDOWN
    \brief 440
 */
/*!
    \var Irc::ERR_USERNOTINCHANNEL
    \brief 441
 */
/*!
    \var Irc::ERR_NOTONCHANNEL
    \brief 442
 */
/*!
    \var Irc::ERR_USERONCHANNEL
    \brief 443
 */
/*!
    \var Irc::ERR_NOLOGIN
    \brief 444
 */
/*!
    \var Irc::ERR_SUMMONDISABLED
    \brief 445
 */
/*!
    \var Irc::ERR_USERSDISABLED
    \brief 446
 */
/*!
    \var Irc::ERR_NONICKCHANGE
    \brief 447
 */
/*!
    \var Irc::ERR_NOTIMPLEMENTED
    \brief 449
 */
/*!
    \var Irc::ERR_NOTREGISTERED
    \brief 451
 */
/*!
    \var Irc::ERR_IDCOLLISION
    \brief 452
 */
/*!
    \var Irc::ERR_NICKLOST
    \brief 453
 */
/*!
    \var Irc::ERR_HOSTILENAME
    \brief 455
 */
/*!
    \var Irc::ERR_ACCEPTFULL
    \brief 456
 */
/*!
    \var Irc::ERR_ACCEPTEXIST
    \brief 457
 */
/*!
    \var Irc::ERR_ACCEPTNOT
    \brief 458
 */
/*!
    \var Irc::ERR_NOHIDING
    \brief 459
 */
/*!
    \var Irc::ERR_NOTFORHALFOPS
    \brief 460
 */
/*!
    \var Irc::ERR_NEEDMOREPARAMS
    \brief 461
 */
/*!
    \var Irc::ERR_ALREADYREGISTERED
    \brief 462
 */
/*!
    \var Irc::ERR_NOPERMFORHOST
    \brief 463
 */
/*!
    \var Irc::ERR_PASSWDMISMATCH
    \brief 464
 */
/*!
    \var Irc::ERR_YOUREBANNEDCREEP
    \brief 465
 */
/*!
    \var Irc::ERR_YOUWILLBEBANNED
    \brief 466
 */
/*!
    \var Irc::ERR_KEYSET
    \brief 467
 */
/*!
    \var Irc::ERR_INVALIDUSERNAME
    \brief 468
 */
/*!
    \var Irc::ERR_ONLYSERVERSCANCHANGE
    \brief 468
 */
/*!
    \var Irc::ERR_LINKSET
    \brief 469
 */
/*!
    \var Irc::ERR_LINKCHANNEL
    \brief 470
 */
/*!
    \var Irc::ERR_KICKEDFROMCHAN
    \brief 470
 */
/*!
    \var Irc::ERR_CHANNELISFULL
    \brief 471
 */
/*!
    \var Irc::ERR_UNKNOWNMODE
    \brief 472
 */
/*!
    \var Irc::ERR_INVITEONLYCHAN
    \brief 473
 */
/*!
    \var Irc::ERR_BANNEDFROMCHAN
    \brief 474
 */
/*!
    \var Irc::ERR_BADCHANNELKEY
    \brief 475
 */
/*!
    \var Irc::ERR_BADCHANMASK
    \brief 476
 */
/*!
    \var Irc::ERR_NOCHANMODES
    \brief 477
 */
/*!
    \var Irc::ERR_NEEDREGGEDNICK
    \brief 477
 */
/*!
    \var Irc::ERR_BANLISTFULL
    \brief 478
 */
/*!
    \var Irc::ERR_BADCHANNAME
    \brief 479
 */
/*!
    \var Irc::ERR_LINKFAIL
    \brief 479
 */
/*!
    \var Irc::ERR_NOULINE
    \brief 480
 */
/*!
    \var Irc::ERR_CANNOTKNOCK
    \brief 480
 */
/*!
    \var Irc::ERR_NOPRIVILEGES
    \brief 481
 */
/*!
    \var Irc::ERR_CHANOPRIVSNEEDED
    \brief 482
 */
/*!
    \var Irc::ERR_CANTKILLSERVER
    \brief 483
 */
/*!
    \var Irc::ERR_RESTRICTED
    \brief 484
 */
/*!
    \var Irc::ERR_ISCHANSERVICE
    \brief 484
 */
/*!
    \var Irc::ERR_DESYNC
    \brief 484
 */
/*!
    \var Irc::ERR_ATTACKDENY
    \brief 484
 */
/*!
    \var Irc::ERR_UNIQOPRIVSNEEDED
    \brief 485
 */
/*!
    \var Irc::ERR_KILLDENY
    \brief 485
 */
/*!
    \var Irc::ERR_CANTKICKADMIN
    \brief 485
 */
/*!
    \var Irc::ERR_ISREALSERVICE
    \brief 485
 */
/*!
    \var Irc::ERR_NONONREG
    \brief 486
 */
/*!
    \var Irc::ERR_HTMDISABLED
    \brief 486
 */
/*!
    \var Irc::ERR_ACCOUNTONLY
    \brief 486
 */
/*!
    \var Irc::ERR_CHANTOORECENT
    \brief 487
 */
/*!
    \var Irc::ERR_MSGSERVICES
    \brief 487
 */
/*!
    \var Irc::ERR_TSLESSCHAN
    \brief 488
 */
/*!
    \var Irc::ERR_VOICENEEDED
    \brief 489
 */
/*!
    \var Irc::ERR_SECUREONLYCHAN
    \brief 489
 */
/*!
    \var Irc::ERR_NOOPERHOST
    \brief 491
 */
/*!
    \var Irc::ERR_NOSERVICEHOST
    \brief 492
 */
/*!
    \var Irc::ERR_NOFEATURE
    \brief 493
 */
/*!
    \var Irc::ERR_BADFEATURE
    \brief 494
 */
/*!
    \var Irc::ERR_BADLOGTYPE
    \brief 495
 */
/*!
    \var Irc::ERR_BADLOGSYS
    \brief 496
 */
/*!
    \var Irc::ERR_BADLOGVALUE
    \brief 497
 */
/*!
    \var Irc::ERR_ISOPERLCHAN
    \brief 498
 */
/*!
    \var Irc::ERR_CHANOWNPRIVNEEDED
    \brief 499
 */
/*!
    \var Irc::ERR_UMODEUNKNOWNFLAG
    \brief 501
 */
/*!
    \var Irc::ERR_USERSDONTMATCH
    \brief 502
 */
/*!
    \var Irc::ERR_GHOSTEDCLIENT
    \brief 503
 */
/*!
    \var Irc::ERR_VWORLDWARN
    \brief 503
 */
/*!
    \var Irc::ERR_USERNOTONSERV
    \brief 504
 */
/*!
    \var Irc::ERR_SILELISTFULL
    \brief 511
 */
/*!
    \var Irc::ERR_TOOMANYWATCH
    \brief 512
 */
/*!
    \var Irc::ERR_BADPING
    \brief 513
 */
/*!
    \var Irc::ERR_INVALID_ERROR
    \brief 514
 */
/*!
    \var Irc::ERR_TOOMANYDCC
    \brief 514
 */
/*!
    \var Irc::ERR_BADEXPIRE
    \brief 515
 */
/*!
    \var Irc::ERR_DONTCHEAT
    \brief 516
 */
/*!
    \var Irc::ERR_DISABLED
    \brief 517
 */
/*!
    \var Irc::ERR_NOINVITE
    \brief 518
 */
/*!
    \var Irc::ERR_LONGMASK
    \brief 518
 */
/*!
    \var Irc::ERR_ADMONLY
    \brief 519
 */
/*!
    \var Irc::ERR_TOOMANYUSERS
    \brief 519
 */
/*!
    \var Irc::ERR_OPERONLY
    \brief 520
 */
/*!
    \var Irc::ERR_MASKTOOWIDE
    \brief 520
 */
/*!
    \var Irc::ERR_WHOTRUNC
    \brief 520
 */
/*!
    \var Irc::ERR_LISTSYNTAX
    \brief 521
 */
/*!
    \var Irc::ERR_WHOSYNTAX
    \brief 522
 */
/*!
    \var Irc::ERR_WHOLIMEXCEED
    \brief 523
 */
/*!
    \var Irc::ERR_QUARANTINED
    \brief 524
 */
/*!
    \var Irc::ERR_OPERSPVERIFY
    \brief 524
 */
/*!
    \var Irc::ERR_REMOTEPFX
    \brief 525
 */
/*!
    \var Irc::ERR_PFXUNROUTABLE
    \brief 526
 */
/*!
    \var Irc::ERR_BADHOSTMASK
    \brief 550
 */
/*!
    \var Irc::ERR_HOSTUNAVAIL
    \brief 551
 */
/*!
    \var Irc::ERR_USINGSLINE
    \brief 552
 */
/*!
    \var Irc::ERR_STATSSLINE
    \brief 553
 */
/*!
    \var Irc::RPL_LOGON
    \brief 600
 */
/*!
    \var Irc::RPL_LOGOFF
    \brief 601
 */
/*!
    \var Irc::RPL_WATCHOFF
    \brief 602
 */
/*!
    \var Irc::RPL_WATCHSTAT
    \brief 603
 */
/*!
    \var Irc::RPL_NOWON
    \brief 604
 */
/*!
    \var Irc::RPL_NOWOFF
    \brief 605
 */
/*!
    \var Irc::RPL_WATCHLIST
    \brief 606
 */
/*!
    \var Irc::RPL_ENDOFWATCHLIST
    \brief 607
 */
/*!
    \var Irc::RPL_WATCHCLEAR
    \brief 608
 */
/*!
    \var Irc::RPL_ISOPER
    \brief 610
 */
/*!
    \var Irc::RPL_ISLOCOP
    \brief 611
 */
/*!
    \var Irc::RPL_ISNOTOPER
    \brief 612
 */
/*!
    \var Irc::RPL_ENDOFISOPER
    \brief 613
 */
/*!
    \var Irc::RPL_DCCSTATUS
    \brief 617
 */
/*!
    \var Irc::RPL_DCCLIST
    \brief 618
 */
/*!
    \var Irc::RPL_ENDOFDCCLIST
    \brief 619
 */
/*!
    \var Irc::RPL_WHOWASHOST
    \brief 619
 */
/*!
    \var Irc::RPL_DCCINFO
    \brief 620
 */
/*!
    \var Irc::RPL_ENDOFO
    \brief 626
 */
/*!
    \var Irc::RPL_SETTINGS
    \brief 630
 */
/*!
    \var Irc::RPL_ENDOFSETTINGS
    \brief 631
 */
/*!
    \var Irc::RPL_DUMPING
    \brief 640
 */
/*!
    \var Irc::RPL_DUMPRPL
    \brief 641
 */
/*!
    \var Irc::RPL_EODUMP
    \brief 642
 */
/*!
    \var Irc::RPL_TRACEROUTE_HOP
    \brief 660
 */
/*!
    \var Irc::RPL_TRACEROUTE_START
    \brief 661
 */
/*!
    \var Irc::RPL_MODECHANGEWARN
    \brief 662
 */
/*!
    \var Irc::RPL_CHANREDIR
    \brief 663
 */
/*!
    \var Irc::RPL_SERVMODEIS
    \brief 664
 */
/*!
    \var Irc::RPL_OTHERUMODEIS
    \brief 665
 */
/*!
    \var Irc::RPL_ENDOF_GENERIC
    \brief 666
 */
/*!
    \var Irc::RPL_WHOWASDETAILS
    \brief 670
 */
/*!
    \var Irc::RPL_WHOISSECURE
    \brief 671
 */
/*!
    \var Irc::RPL_UNKNOWNMODES
    \brief 672
 */
/*!
    \var Irc::RPL_CANNOTSETMODES
    \brief 673
 */
/*!
    \var Irc::RPL_LUSERSTAFF
    \brief 678
 */
/*!
    \var Irc::RPL_TIMEONSERVERIS
    \brief 679
 */
/*!
    \var Irc::RPL_NETWORKS
    \brief 682
 */
/*!
    \var Irc::RPL_YOURLANGUAGEIS
    \brief 687
 */
/*!
    \var Irc::RPL_LANGUAGE
    \brief 688
 */
/*!
    \var Irc::RPL_WHOISSTAFF
    \brief 689
 */
/*!
    \var Irc::RPL_WHOISLANGUAGE
    \brief 690
 */
/*!
    \var Irc::RPL_HELPSTART
    \brief 704
 */
/*!
    \var Irc::RPL_HELPTXT
    \brief 705
 */
/*!
    \var Irc::RPL_ENDOFHELP
    \brief 706
 */
/*!
    \var Irc::RPL_ETRACEFULL
    \brief 708
 */
/*!
    \var Irc::RPL_ETRACE
    \brief 709
 */
/*!
    \var Irc::RPL_KNOCK
    \brief 710
 */
/*!
    \var Irc::RPL_KNOCKDLVR
    \brief 711
 */
/*!
    \var Irc::ERR_TOOMANYKNOCK
    \brief 712
 */
/*!
    \var Irc::ERR_CHANOPEN
    \brief 713
 */
/*!
    \var Irc::ERR_KNOCKONCHAN
    \brief 714
 */
/*!
    \var Irc::ERR_KNOCKDISABLED
    \brief 715
 */
/*!
    \var Irc::RPL_TARGUMODEG
    \brief 716
 */
/*!
    \var Irc::RPL_TARGNOTIFY
    \brief 717
 */
/*!
    \var Irc::RPL_UMODEGMSG
    \brief 718
 */
/*!
    \var Irc::RPL_ENDOFOMOTD
    \brief 722
 */
/*!
    \var Irc::ERR_NOPRIVS
    \brief 723
 */
/*!
    \var Irc::RPL_TESTMARK
    \brief 724
 */
/*!
    \var Irc::RPL_TESTLINE
    \brief 725
 */
/*!
    \var Irc::RPL_NOTESTLINE
    \brief 726
 */
/*!
    \var Irc::RPL_XINFO
    \brief 771
 */
/*!
    \var Irc::RPL_XINFOSTART
    \brief 773
 */
/*!
    \var Irc::RPL_XINFOEND
    \brief 774
 */
/*!
    \var Irc::ERR_CANNOTDOCOMMAND
    \brief 972
 */
/*!
    \var Irc::ERR_CANNOTCHANGEUMODE
    \brief 973
 */
/*!
    \var Irc::ERR_CANNOTCHANGECHANMODE
    \brief 974
 */
/*!
    \var Irc::ERR_CANNOTCHANGESERVERMODE
    \brief 975
 */
/*!
    \var Irc::ERR_CANNOTSENDTONICK
    \brief 976
 */
/*!
    \var Irc::ERR_UNKNOWNSERVERMODE
    \brief 977
 */
/*!
    \var Irc::ERR_SERVERMODELOCK
    \brief 979
 */
/*!
    \var Irc::ERR_BADCHARENCODING
    \brief 980
 */
/*!
    \var Irc::ERR_TOOMANYLANGUAGES
    \brief 981
 */
/*!
    \var Irc::ERR_NOLANGUAGE
    \brief 982
 */
/*!
    \var Irc::ERR_TEXTTOOSHORT
    \brief 983
 */
/*!
    \var Irc::ERR_NUMERIC_ERR
    \brief 999
 */
