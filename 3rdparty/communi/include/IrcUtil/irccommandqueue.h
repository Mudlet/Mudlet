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

#ifndef IRCCOMMANDQUEUE_H
#define IRCCOMMANDQUEUE_H

#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qscopedpointer.h>

IRC_BEGIN_NAMESPACE

class IrcConnection;
class IrcCommandQueuePrivate;

class IRC_UTIL_EXPORT IrcCommandQueue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int batch READ batch WRITE setBatch)
    Q_PROPERTY(int interval READ interval WRITE setInterval)
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)
    Q_PROPERTY(IrcConnection* connection READ connection WRITE setConnection)

public:
    explicit IrcCommandQueue(QObject* parent = 0);
    virtual ~IrcCommandQueue();

    int batch() const;
    void setBatch(int batch);

    int interval() const;
    void setInterval(int seconds);

    int size() const;

    IrcConnection* connection() const;
    void setConnection(IrcConnection* connection);

public Q_SLOTS:
    void clear();
    void flush();

Q_SIGNALS:
    void sizeChanged(int size);

private:
    QScopedPointer<IrcCommandQueuePrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcCommandQueue)
    Q_DISABLE_COPY(IrcCommandQueue)

    Q_PRIVATE_SLOT(d_func(), void _irc_updateTimer())
    Q_PRIVATE_SLOT(d_func(), void _irc_sendBatch())
};

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCommandQueue*))

#endif // IRCCOMMANDQUEUE_H
