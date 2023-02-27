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

#ifndef IRCGLOBAL_H
#define IRCGLOBAL_H

#include <QtCore/qglobal.h>

/*!
    \file ircglobal.h
    \brief \#include &lt;\ref ircglobal.h "IrcGlobal"&gt;
 */

#if defined(IRC_SHARED)
#
#  if defined(BUILD_IRC_CORE)
#    define IRC_CORE_EXPORT Q_DECL_EXPORT
#  else
#    define IRC_CORE_EXPORT Q_DECL_IMPORT
#  endif
#
#  if defined(BUILD_IRC_MODEL)
#    define IRC_MODEL_EXPORT Q_DECL_EXPORT
#  else
#    define IRC_MODEL_EXPORT Q_DECL_IMPORT
#  endif
#
#  if defined(BUILD_IRC_UTIL)
#    define IRC_UTIL_EXPORT Q_DECL_EXPORT
#  else
#    define IRC_UTIL_EXPORT Q_DECL_IMPORT
#  endif
#
#elif defined(IRC_STATIC) || defined(BUILD_IRC_CORE) || defined(BUILD_IRC_MODEL) || defined(BUILD_IRC_UTIL)
#
#    define IRC_CORE_EXPORT
#    define IRC_MODEL_EXPORT
#    define IRC_UTIL_EXPORT
#
#else
#  error Installation problem: either IRC_SHARED or IRC_STATIC must be defined!
#endif

/*!
    \def IRC_VERSION

    This macro expands a numeric value of the form 0xMMNNPP (MM = major, NN = minor, PP = patch) that specifies Communi's version number.
    For example, if you compile your application against Communi 1.2.3, the IRC_VERSION macro will expand to 0x010203.

    You can use IRC_VERSION to use the latest Communi features where available. For example:
    \code
 #if IRC_VERSION >= 0x000300
     // SSL support since version 0.3.0
     connection->setSocket(new QSslSocket(connection));
 #endif
    \endcode

    \sa Irc::version()
 */
#define IRC_VERSION 0x030700

/*!
    \def IRC_VERSION_STR

    This macro expands to a string that specifies Communi's version number (for example, "1.2.3").
    This is the version against which the application is compiled.

    \sa Irc::version()
 */
#define IRC_VERSION_STR "3.7.0"

#ifdef IRC_NAMESPACE

# define IRC_PREPEND_NAMESPACE(name) ::IRC_NAMESPACE::name
# define IRC_USE_NAMESPACE using namespace ::IRC_NAMESPACE;
# define IRC_BEGIN_NAMESPACE namespace IRC_NAMESPACE {
# define IRC_END_NAMESPACE }
# define IRC_FORWARD_DECLARE_CLASS(name) \
    IRC_BEGIN_NAMESPACE class name; IRC_END_NAMESPACE \
    using IRC_PREPEND_NAMESPACE(name);

#else

# define IRC_PREPEND_NAMESPACE(name) ::name
# define IRC_USE_NAMESPACE
# define IRC_BEGIN_NAMESPACE
# define IRC_END_NAMESPACE
# define IRC_FORWARD_DECLARE_CLASS(name) class name;

#endif // IRC_NAMESPACE

#endif // IRCGLOBAL_H
