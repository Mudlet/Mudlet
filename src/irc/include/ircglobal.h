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

#ifndef IRCGLOBAL_H
#define IRCGLOBAL_H

#include <QtCore/qglobal.h>

/*!
    \file ircglobal.h
    \brief #include &lt;IrcGlobal&gt;
 */
#define COMMUNI_STATIC
#if defined(COMMUNI_SHARED)
#  if defined(BUILD_COMMUNI)
#    define COMMUNI_EXPORT Q_DECL_EXPORT
#  else
#    define COMMUNI_EXPORT Q_DECL_IMPORT
#  endif
#elif defined(COMMUNI_STATIC)
#  define COMMUNI_EXPORT
#else
#  error Installation problem: either COMMUNI_SHARED or COMMUNI_STATIC must be defined!
#endif

/*!
    \def COMMUNI_VERSION

    This macro expands a numeric value of the form 0xMMNNPP (MM = major, NN = minor, PP = patch) that specifies Communi's version number.
    For example, if you compile your application against Communi 1.2.3, the COMMUNI_VERSION macro will expand to 0x010203.

    You can use COMMUNI_VERSION to use the latest Communi features where available. For example:
    \code
#if COMMUNI_VERSION >= 0x000300
    // SSL support since version 0.3.0
    session->setSocket(new QSslSocket(session));
#endif
    \endcode
    
    \sa COMMUNI_VERSION_STR and Irc::version().
 */
#define COMMUNI_VERSION 0x010201

/*!
    \def COMMUNI_VERSION_STR

    This macro expands to a string that specifies Communi's version number (for example, "1.2.3").
    This is the version against which the application is compiled.

    \sa Irc::version() and COMMUNI_VERSION.
 */
#define COMMUNI_VERSION_STR "1.2.1"

#ifndef QT_FORWARD_DECLARE_CLASS
#   define QT_FORWARD_DECLARE_CLASS(name) class name;
#endif // QT_FORWARD_DECLARE_CLASS

#endif // IRCGLOBAL_H
