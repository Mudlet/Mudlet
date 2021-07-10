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

#ifndef IRCCORE_P_H
#define IRCCORE_P_H

#include "ircglobal.h"

#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>

IRC_BEGIN_NAMESPACE

namespace IrcPrivate {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    template <typename T>
    QSet<T> listToSet(const QList<T> &list) { return QSet<T>(list.cbegin(), list.cend()); }
    template <typename T>
    static inline QList<T> setToList(const QSet<T> &set) { return QList<T>(set.cbegin(), set.cend()); }
#else
    template <typename T>
    static inline QSet<T> listToSet(const QList<T> &list) { return list.toSet(); }
    template <typename T>
    static inline QList<T> setToList(const QSet<T> &set) { return set.toList(); }
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace Qt {
    const QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
}
#endif

#ifndef Q_FALLTHROUGH
#   if defined(__cplusplus)
#       if __has_cpp_attribute(clang::fallthrough)
#           define Q_FALLTHROUGH() [[clang::fallthrough]]
#       elif __has_cpp_attribute(gnu::fallthrough)
#           define Q_FALLTHROUGH() [[gnu::fallthrough]]
#       elif __has_cpp_attribute(fallthrough)
#           define Q_FALLTHROUGH() [[fallthrough]]
#       endif
#   endif
#endif

#ifndef Q_FALLTHROUGH
#   if (defined(Q_CC_GNU) && Q_CC_GNU >= 700) && !defined(Q_CC_INTEL)
#       define Q_FALLTHROUGH() __attribute__((fallthrough))
#   else
#       define Q_FALLTHROUGH() (void)0
#   endif
#endif

IRC_END_NAMESPACE

#endif // IRCCORE_P_H
