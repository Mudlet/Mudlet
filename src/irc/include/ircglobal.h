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

#ifndef IRC_GLOBAL_H
#define IRC_GLOBAL_H

#include <QtGlobal>

/*!
    \file ircglobal.h
 */
#define IRC_STATIC
#if defined(IRC_SHARED)
#  if defined(BUILD_IRC)
#    define IRC_EXPORT Q_DECL_EXPORT
#  else
#    define IRC_EXPORT Q_DECL_IMPORT
#  endif
#elif defined(IRC_STATIC)
#  define IRC_EXPORT
#else
#  error Installation problem: either IRC_SHARED or IRC_STATIC must be defined!
#endif
/*!
    \def IRC_VERSION

    This macro expands a numeric value of the form 0xMMNNPP (MM = major, NN = minor, PP = patch) that specifies LibIrcClient-Qt's version number.
    For example, if you compile your application against LibIrcClient-Qt 1.2.3, the IRC_VERSION macro will expand to 0x010203.

    You can use IRC_VERSION to use the latest LibIrcClient-Qt features where available. For example:
    \code
#if IRC_VERSION >= 0x000300
    session->setSocket(new QSslSocket(session));
    session->connectToServer(host, port);
#endif
    \endcode
    
    \sa IRC_VERSION_STR and Irc::version().
 */
#define IRC_VERSION 0x000500

/*!
    \def IRC_VERSION_STR

    This macro expands to a string that specifies LibIrcClient-Qt's version number (for example, "1.2.3").
    This is the version against which the application is compiled.

    \sa Irc::version() and IRC_VERSION.
 */
#define IRC_VERSION_STR "0.5.0"

#ifndef QT_FORWARD_DECLARE_CLASS
#   define QT_FORWARD_DECLARE_CLASS(name) class name;
#endif // QT_FORWARD_DECLARE_CLASS

#endif // IRC_GLOBAL_H
