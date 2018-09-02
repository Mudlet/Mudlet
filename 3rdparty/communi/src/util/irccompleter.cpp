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

#include "irccompleter.h"
#include "irccommandparser.h"
#include "irccommandparser_p.h"
#include "ircbuffermodel.h"
#include "ircusermodel.h"
#include "ircnetwork.h"
#include "ircchannel.h"
#include "irctoken_p.h"
#include "ircuser.h"

#include <QTextBoundaryFinder>
#include <QPointer>
#include <QList>
#include <QPair>

IRC_BEGIN_NAMESPACE

/*!
    \file irccompleter.h
    \brief \#include &lt;IrcCompleter&gt;
 */

/*!
    \since 3.1
    \class IrcCompleter irccompleter.h <IrcCompleter>
    \ingroup util
    \brief Provides command and name completion.

    IrcCompleter provides command and name completion for a text input field. The completer
    is made context aware by assigning a command \ref IrcCompleter::parser "parser" and a
    \ref buffer that is currently active in the GUI. The parser is used for completing
    commands, and the buffer is used for completing buffer and user names.

    In order to perform a completion, call complete() with the current text input field
    content and the cursor position. If a suitable completion is found, the completed()
    signal is emitted with a suggestion for a new content and cursor position for the
    text input field.

    \code
    TextField {
        id: textField

        Keys.onTabPressed: completer.complete(text, cursorPosition)

        IrcCompleter {
            id: completer

            buffer: ...
            parser: ...

            onCompleted: {
                textField.text = text
                textField.cursorPosition = cursor
            }
        }
    }
    \endcode

    \sa IrcCommandParser, IrcBuffer
 */

/*!
    \fn void IrcCompleter::completed(const QString& text, int cursor)

    This signal is emitted when a suitable completion with \a text and \a cursor position is found.
 */

#ifndef IRC_DOXYGEN

static bool isPrefixed(const QString& text, int pos, const QStringList& prefixes, int* len)
{
    foreach (const QString& prefix, prefixes) {
        const int ll = prefix.length();
        if (text.mid(pos, ll) == prefix) {
            if (len)
                *len = 0;
            return true;
        } else if (text.mid(pos - ll, ll) == prefix) {
            if (len)
                *len = ll;
            return true;
        }
    }
    return false;
}

struct IrcCompletion
{
    IrcCompletion() : text(), cursor(-1) { }
    IrcCompletion(const QString& txt, int pos) : text(txt), cursor(pos) { }
    bool isValid() const { return !text.isNull() && cursor != -1; }
    bool operator ==(const IrcCompletion& other) const { return text == other.text && cursor == other.cursor; }
    bool operator !=(const IrcCompletion& other) const { return text != other.text || cursor != other.cursor; }
    QString text;
    int cursor;
};

class IrcCompleterPrivate
{
    Q_DECLARE_PUBLIC(IrcCompleter)

public:
    IrcCompleterPrivate();

    void completeNext(IrcCompleter::Direction direction);
    QList<IrcCompletion> completeCommands(const QString& text, int pos) const;
    QList<IrcCompletion> completeWords(const QString& text, int pos) const;

    IrcCompleter* q_ptr;

    int index;
    int cursor;
    QString text;
    QList<IrcCompletion> completions;

    QString suffix;
    QPointer<IrcBuffer> buffer;
    QPointer<IrcCommandParser> parser;
};

IrcCompleterPrivate::IrcCompleterPrivate() : q_ptr(0), index(-1), cursor(-1), suffix(":"), buffer(0), parser(0)
{
}

void IrcCompleterPrivate::completeNext(IrcCompleter::Direction direction)
{
    Q_Q(IrcCompleter);
    Q_ASSERT(!completions.isEmpty());
    if (direction == IrcCompleter::Forward) {
        index = (index + 1) % completions.length();
    } else {
        if (--index < 0)
            index = completions.length() - 1;
    }
    if (index >= 0 && index < completions.length()) {
        const IrcCompletion completion = completions.at(index);
        text = completion.text;
        cursor = completion.cursor;
        emit q->completed(text, cursor);
    }
}

static IrcCompletion completeCommand(const QString& text, const QString& command)
{
    IrcTokenizer tokenizer(text);
    tokenizer.replace(0, command);
    QString completion = tokenizer.toString();
    int next = command.length();
    if (next >= completion.length() || completion.at(next) != QLatin1Char(' '))
        completion.insert(next, QLatin1Char(' '));
    return IrcCompletion(completion, ++next);
}

QList<IrcCompletion> IrcCompleterPrivate::completeCommands(const QString& text, int pos) const
{
    if (!parser)
        return QList<IrcCompletion>();

    QList<IrcCompletion> completions;

    int removed = 0;
    QString input = text;
    IrcCommandParserPrivate* pp = IrcCommandParserPrivate::get(parser);
    if (pp->processCommand(&input, &removed)) {
        const QString command = input.split(QLatin1Char(' '), QString::SkipEmptyParts).value(0).toUpper();
        if (!command.isEmpty()) {
            foreach (const IrcCommandInfo& cmd, pp->commands) {
                if (cmd.command == command)
                    return QList<IrcCompletion>() << completeCommand(text, text.left(removed) + cmd.command);
                if (cmd.command.startsWith(command))
                    completions += completeCommand(text, text.left(removed) + cmd.command);
            }
        }
        // TODO: context sensitive command parameter completion
        Q_UNUSED(pos);
    }
    return completions;
}

static IrcCompletion completeWord(const QString& text, int from, int len, const QString& word)
{
    QString completion = QString(text).replace(from, len, word);
    int next = from + word.length();
    if (next >= completion.length() || completion.at(next) != QLatin1Char(' '))
        completion.insert(next, QLatin1Char(' '));
    return IrcCompletion(completion, ++next);
}

QList<IrcCompletion> IrcCompleterPrivate::completeWords(const QString& text, int pos) const
{
    if (!buffer || !buffer->network())
        return QList<IrcCompletion>();

    QList<IrcCompletion> completions;

    const IrcToken token = IrcTokenizer(text).find(pos);
    const QPair<int, int> bounds = qMakePair(token.position(), token.length());
    if (bounds.first != -1 && bounds.second != -1) {
        const QString word = text.mid(bounds.first, bounds.second);

        int pfx = 0;
        QString prefix;
        bool isChannel = isPrefixed(text, bounds.first, buffer->network()->channelTypes(), &pfx);
        if (isChannel && pfx > 0)
            prefix = text.mid(bounds.first - pfx, pfx);

        if (!isChannel) {
            IrcUserModel userModel;
            userModel.setSortMethod(Irc::SortByActivity);
            userModel.setChannel(qobject_cast<IrcChannel*>(buffer));
            foreach (IrcUser* user, userModel.users()) {
                if (user->name().startsWith(word, Qt::CaseInsensitive)) {
                    QString name = user->name();
                    if (token.index() == 0)
                        name += suffix;
                    IrcCompletion completion = completeWord(text, bounds.first, bounds.second, name);
                    if (completion.isValid() && !completions.contains(completion))
                        completions += completion;
                }
            }
        }

        QList<IrcBuffer*> buffers = buffer->model()->buffers();
        buffers.move(buffers.indexOf(buffer), 0); // promote the current buffer
        foreach (IrcBuffer* buffer, buffers) {
            if (!buffer->isChannel()) {
                // it would be very confusing to auto-complete the titles of other
                // open query (or server) buffers, because it makes it look like such
                // user would be on the channel
                continue;
            }
            QString title = buffer->title();
            if (!isChannel && token.index() == 0)
                title += suffix;
            IrcCompletion completion;
            if (title.startsWith(word, Qt::CaseInsensitive))
                completion = completeWord(text, bounds.first, bounds.second, title);
            else if (isChannel && !prefix.isEmpty() && title.startsWith(prefix + word, Qt::CaseInsensitive))
                completion = completeWord(text, bounds.first - prefix.length(), bounds.second + prefix.length(), title);
            if (completion.isValid() && !completions.contains(completion))
                completions += completion;
        }
    }
    return completions;
}
#endif // IRC_DOXYGEN

/*!
    Constructs a completer with \a parent.
 */
IrcCompleter::IrcCompleter(QObject* parent) : QObject(parent), d_ptr(new IrcCompleterPrivate)
{
    Q_D(IrcCompleter);
    d->q_ptr = this;
}

/*!
    Destructs the completer.
 */
IrcCompleter::~IrcCompleter()
{
}

/*!
    This property holds the completion suffix.

    The suffix is appended to the end of a completed nick name, but
    only when the nick name is in the beginning of completed text.

    The default value is \c ":".

    \par Access functions:
    \li QString <b>suffix</b>() const
    \li void <b>setSuffix</b>(const QString& suffix) [slot]

    \par Notifier signal:
    \li void <b>suffixChanged</b>(const QString& suffix)
 */
QString IrcCompleter::suffix() const
{
    Q_D(const IrcCompleter);
    return d->suffix;
}

void IrcCompleter::setSuffix(const QString& suffix)
{
    Q_D(IrcCompleter);
    if (d->suffix != suffix) {
        d->suffix = suffix;
        emit suffixChanged(suffix);
    }
}

/*!
    This property holds the buffer used for name completion.

    \par Access functions:
    \li \ref IrcBuffer* <b>buffer</b>() const
    \li void <b>setBuffer</b>(\ref IrcBuffer* buffer) [slot]

    \par Notifier signal:
    \li void <b>bufferChanged</b>(\ref IrcBuffer* buffer)
 */
IrcBuffer* IrcCompleter::buffer() const
{
    Q_D(const IrcCompleter);
    return d->buffer;
}

void IrcCompleter::setBuffer(IrcBuffer* buffer)
{
    Q_D(IrcCompleter);
    if (d->buffer != buffer) {
        d->buffer = buffer;
        emit bufferChanged(buffer);
    }
}

/*!
    This property holds the parser used for command completion.

    \par Access functions:
    \li \ref IrcCommandParser* <b>parser</b>() const
    \li void <b>setParser</b>(\ref IrcCommandParser* parser) [slot]

    \par Notifier signal:
    \li void <b>parserChanged</b>(\ref IrcCommandParser* parser)
 */
IrcCommandParser* IrcCompleter::parser() const
{
    Q_D(const IrcCompleter);
    return d->parser;
}

void IrcCompleter::setParser(IrcCommandParser* parser)
{
    Q_D(IrcCompleter);
    if (d->parser != parser) {
        d->parser = parser;
        emit parserChanged(parser);
    }
}

/*!
    Completes \a text at \a cursor position, iterating multiple
    matches to the specified \a direction, and emits completed()
    if a suitable completion is found.
 */
void IrcCompleter::complete(const QString& text, int cursor, Direction direction)
{
    Q_D(IrcCompleter);
    if (!d->completions.isEmpty() && d->cursor == cursor && d->text == text) {
        d->completeNext(direction);
        return;
    }

    QList<IrcCompletion> completions = d->completeCommands(text, cursor);
    if (completions.isEmpty() || IrcTokenizer(text).find(cursor).index() > 0)
        completions = d->completeWords(text, cursor);

    if (d->completions != completions) {
        d->index = -1;
        d->completions = completions;
    }
    if (!d->completions.isEmpty())
        d->completeNext(direction);
}

/*!
    Resets the completer state.
 */
void IrcCompleter::reset()
{
    Q_D(IrcCompleter);
    d->index = -1;
    d->cursor = -1;
    d->text.clear();
    d->completions.clear();
}

#include "moc_irccompleter.cpp"

IRC_END_NAMESPACE
