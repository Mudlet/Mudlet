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

#ifndef IRCCOMMANDPARSER_H
#define IRCCOMMANDPARSER_H

#include <IrcGlobal>
#include <IrcCommand>
#include <QtCore/qobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstringlist.h>

IRC_BEGIN_NAMESPACE

class IrcCommandParserPrivate;

class IRC_UTIL_EXPORT IrcCommandParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QStringList triggers READ triggers WRITE setTriggers NOTIFY triggersChanged)
    Q_PROPERTY(QStringList channels READ channels WRITE setChannels NOTIFY channelsChanged)
    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool tolerant READ isTolerant WRITE setTolerant NOTIFY tolerancyChanged)
    Q_FLAGS(Details)

public:
    explicit IrcCommandParser(QObject* parent = 0);
    virtual ~IrcCommandParser();

    QStringList commands() const;

    enum Detail {
        Full = 0x0,
        NoTarget = 0x1,
        NoPrefix = 0x2,
        NoEllipsis = 0x4,
        NoParentheses = 0x8,
        NoBrackets = 0x10,
        NoAngles = 0x20,
        Visual = NoTarget | NoPrefix | NoEllipsis
    };
    Q_DECLARE_FLAGS(Details, Detail)

    Q_INVOKABLE QString syntax(const QString& command, Details details = Visual) const;

    Q_INVOKABLE void addCommand(IrcCommand::Type type, const QString& syntax);
    Q_INVOKABLE void removeCommand(IrcCommand::Type type, const QString& syntax = QString());

    QStringList triggers() const;

    QString target() const;
    QStringList channels() const;

    bool isTolerant() const;
    void setTolerant(bool tolerant);

    Q_INVOKABLE IrcCommand* parse(const QString& input) const;

public Q_SLOTS:
    void clear();
    void reset();

    void setTriggers(const QStringList& triggers);
    void setChannels(const QStringList& channels);
    void setTarget(const QString& target);

Q_SIGNALS:
    void commandsChanged(const QStringList& commands);
    void triggersChanged(const QStringList& triggers);
    void channelsChanged(const QStringList& channels);
    void targetChanged(const QString& target);
    void tolerancyChanged(bool tolerant);

private:
    QScopedPointer<IrcCommandParserPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcCommandParser)
    Q_DISABLE_COPY(IrcCommandParser)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IrcCommandParser::Details)

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCommandParser*))

#endif // IRCCOMMANDPARSER_H
