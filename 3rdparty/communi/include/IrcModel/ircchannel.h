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

#ifndef IRCCHANNEL_H
#define IRCCHANNEL_H

#include <IrcGlobal>
#include <IrcBuffer>
#include <QtCore/qmetatype.h>

IRC_BEGIN_NAMESPACE

class IrcChannelPrivate;

class IRC_MODEL_EXPORT IrcChannel : public IrcBuffer
{
    Q_OBJECT
    Q_PROPERTY(QString key READ key NOTIFY keyChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString topic READ topic NOTIFY topicChanged)

public:
    Q_INVOKABLE explicit IrcChannel(QObject* parent = 0);
    virtual ~IrcChannel();

    QString key() const;
    QString mode() const;
    QString topic() const;

    virtual bool isActive() const;

public Q_SLOTS:
    void who();
    void join(const QString& key = QString());
    void part(const QString& reason = QString());
    void close(const QString& reason = QString());

Q_SIGNALS:
    void keyChanged(const QString& key);
    void modeChanged(const QString& mode);
    void topicChanged(const QString& topic);
    void destroyed(IrcChannel* channel);
private:
    Q_DECLARE_PRIVATE(IrcChannel)
    Q_DISABLE_COPY(IrcChannel)
};

#ifndef QT_NO_DEBUG_STREAM
IRC_MODEL_EXPORT QDebug operator<<(QDebug debug, const IrcChannel* channel);
#endif // QT_NO_DEBUG_STREAM

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcChannel*))
Q_DECLARE_METATYPE(QList<IRC_PREPEND_NAMESPACE(IrcChannel*)>)

#endif // IRCCHANNEL
