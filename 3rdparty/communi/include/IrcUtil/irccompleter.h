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

#ifndef IRCCOMPLETER_H
#define IRCCOMPLETER_H

#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qmetatype.h>

IRC_BEGIN_NAMESPACE

class IrcBuffer;
class IrcCommandParser;
class IrcCompleterPrivate;

class IRC_UTIL_EXPORT IrcCompleter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix NOTIFY suffixChanged)
    Q_PROPERTY(IrcBuffer* buffer READ buffer WRITE setBuffer NOTIFY bufferChanged)
    Q_PROPERTY(IrcCommandParser* parser READ parser WRITE setParser NOTIFY parserChanged)
    Q_ENUMS(Direction)

public:
    explicit IrcCompleter(QObject* parent = 0);
    virtual ~IrcCompleter();

    enum Direction {
        Forward,
        Backward
    };

    QString suffix() const;
    IrcBuffer* buffer() const;
    IrcCommandParser* parser() const;

public Q_SLOTS:
    void setSuffix(const QString& suffix);
    void setBuffer(IrcBuffer* buffer);
    void setParser(IrcCommandParser* parser);

    void complete(const QString& text, int cursor, Direction direction = Forward);
    void reset();

Q_SIGNALS:
    void suffixChanged(const QString& suffix);
    void bufferChanged(IrcBuffer* buffer);
    void parserChanged(IrcCommandParser* parser);

    void completed(const QString& text, int cursor);

private:
    QScopedPointer<IrcCompleterPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcCompleter)
    Q_DISABLE_COPY(IrcCompleter)
};

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCompleter*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCompleter::Direction))

#endif // IRCCOMPLETER_H
