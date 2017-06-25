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

#ifndef IRCUSER_H
#define IRCUSER_H

#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qscopedpointer.h>

IRC_BEGIN_NAMESPACE

class IrcChannel;
class IrcUserPrivate;

class IRC_MODEL_EXPORT IrcUser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString prefix READ prefix NOTIFY prefixChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(bool servOp READ isServOp NOTIFY servOpChanged)
    Q_PROPERTY(bool away READ isAway NOTIFY awayChanged)
    Q_PROPERTY(IrcChannel* channel READ channel CONSTANT)

public:
    explicit IrcUser(QObject* parent = 0);
    virtual ~IrcUser();

    QString title() const;
    QString name() const;
    QString prefix() const;
    QString mode() const;
    bool isServOp() const;
    bool isAway() const;

    IrcChannel* channel() const;

Q_SIGNALS:
    void titleChanged(const QString& title);
    void nameChanged(const QString& name);
    void prefixChanged(const QString& prefix);
    void modeChanged(const QString& mode);
    void servOpChanged(bool servOp);
    void awayChanged(bool away);

private:
    QScopedPointer<IrcUserPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcUser)
    Q_DISABLE_COPY(IrcUser)
};

#ifndef QT_NO_DEBUG_STREAM
IRC_MODEL_EXPORT QDebug operator<<(QDebug debug, const IrcUser* user);
#endif // QT_NO_DEBUG_STREAM

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcUser*))
Q_DECLARE_METATYPE(QList<IRC_PREPEND_NAMESPACE(IrcUser*)>)

#endif // IRCUSER_H
