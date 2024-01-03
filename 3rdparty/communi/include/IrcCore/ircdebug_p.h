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

#ifndef IRCDEBUG_P_H
#define IRCDEBUG_P_H

#include <IrcGlobal>
#include <IrcConnection>
#include <QtCore/qdebug.h>
#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

IRC_BEGIN_NAMESPACE

#ifndef IRC_DOXYGEN
static bool irc_debug_enabled(IrcConnection* c, uint l);

class IrcDebug
{
public:
    enum Level { None, Error, Status, Write, Read };

    IrcDebug(IrcConnection* c, Level l) : enabled(irc_debug_enabled(c, l))
#ifndef QT_NO_DEBUG_STREAM
      , debug(&str)
#endif // QT_NO_DEBUG_STREAM
    {
#ifndef QT_NO_DEBUG_STREAM
        if (enabled) {
            const QString desc = c->displayName();
            const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
            debug << qPrintable("[" + stamp + " " + desc + "]");
            switch (l) {
                case Error: debug << "!!"; break;
                case Status: debug << "??"; break;
                case Write: debug << "->"; break;
                case Read: debug << "<-"; break;
                default: break;
            }
        }
#endif // QT_NO_DEBUG_STREAM
    }

    ~IrcDebug() {
#ifndef QT_NO_DEBUG_STREAM
        if (enabled)
            qDebug() << qPrintable(str);
#endif // QT_NO_DEBUG_STREAM
    }

    template<typename T>
    inline IrcDebug &operator<<(const T& t)
    {
#ifdef QT_NO_DEBUG_STREAM
        Q_UNUSED(t);
#else
        if (enabled) {
            debug << t;
        }
#endif // QT_NO_DEBUG_STREAM
        return *this;
    }

private:
    bool enabled;
    QString str;
#ifndef QT_NO_DEBUG_STREAM
    QDebug debug;
#endif // QT_NO_DEBUG_STREAM
};

static bool irc_debug_enabled(IrcConnection* c, uint l)
{
    static QString dbg_name;
    static uint dbg_level = IrcDebug::None;

    static bool dbg_init = false;
    if (!dbg_init) {
        QByteArray lenv = qgetenv("IRC_DEBUG_LEVEL").toLower();
        if (!lenv.isEmpty()) {
            bool ok = false;
            int number = lenv.toInt(&ok);
            if (ok) {
                dbg_level = number;
            } else if (lenv == "none") {
                dbg_level = IrcDebug::None;
            } else if (lenv == "error") {
                dbg_level = IrcDebug::Error;
            } else if (lenv == "status") {
                dbg_level = IrcDebug::Status;
            } else if (lenv == "write") {
                dbg_level = IrcDebug::Write;
            } else if (lenv == "read") {
                dbg_level = IrcDebug::Read;
            } else {
                qWarning("Unknown IRC_DEBUG_LEVEL value '%s'", lenv.data());
                qWarning("Available values: 0-4, none, error, status, write, read.");
            }
        }

        QByteArray denv = qgetenv("IRC_DEBUG");
        if (!denv.isEmpty()) {
            bool ok = false;
            int number = denv.toInt(&ok);
            if (ok) {
                if (number == 0)
                    dbg_level = IrcDebug::None;
                else if (lenv.isEmpty())
                    dbg_level = IrcDebug::Read;
            }
        }

        dbg_name = QString::fromUtf8(qgetenv("IRC_DEBUG_NAME"));
        if (!dbg_name.isEmpty() && lenv.isEmpty() && denv.isEmpty())
            dbg_level = IrcDebug::Read;

        dbg_init = true;
    }
    if (l <= dbg_level) {
        if (dbg_name.isEmpty())
            return true;

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        return QRegularExpression(QRegularExpression::wildcardToRegularExpression(dbg_name),
                       QRegularExpression::CaseInsensitiveOption).match(c->displayName()).hasMatch();
#else
        return QRegExp(dbg_name,
                   Qt::CaseInsensitive,
                   QRegExp::Wildcard).exactMatch(c->displayName());
#endif
    }
    return false;
}

#define ircDebug(Connection, Flag) IrcDebug(Connection, Flag)
#endif // IRC_DOXYGEN

IRC_END_NAMESPACE

#endif // IRCDEBUG_P_H
