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

#include "irccommandparser.h"
#include "irccommandparser_p.h"
#include "irctoken_p.h"
#include <climits>

IRC_BEGIN_NAMESPACE

/*!
    \file irccommandparser.h
    \brief \#include &lt;IrcCommandParser&gt;
 */

/*!
    \class IrcCommandParser irccommandparser.h <IrcCommandParser>
    \ingroup util
    \brief Parses commands from user input.

    \section syntax Syntax

    Since the list of supported commands and the exact syntax for each
    command is application specific, IrcCommandParser does not provide
    any built-in command syntaxes. It is left up to the applications
    to introduce the supported commands and syntaxes.
    IrcCommandParser supports the following command syntax markup:

    Syntax             | Example              | Description
    -------------------|----------------------|------------
    &lt;param&gt;      | &lt;target&gt;       | A required parameter.
    (&lt;param&gt;)    | (&lt;key&gt;)        | An optional parameter.
    &lt;param...&gt;   | &lt;message...&gt;   | A required parameter, multiple words accepted. (1)
    (&lt;param...&gt;) | (&lt;message...&gt;) | An optional parameter, multiple words accepted. (1)
    (&lt;\#param&gt;)  | (&lt;\#channel&gt;)  | An optional channel parameter. (2)
    [param]            | [target]             | Inject the current target.

    -# Multi-word parameters are only supported in the last parameter position.
    -# An optional channel parameter is filled up with the current channel when absent.

    The following example presents introducing some typical commands.
    \code
    IrcCommandParser* parser = new IrcCommandParser(this);
    parser->addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
    parser->addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
    parser->addCommand(IrcCommand::Kick, "KICK (<#channel>) <nick> (<reason...>)");
    parser->addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
    parser->addCommand(IrcCommand::CtcpAction, "ACTION <target> <message...>");
    \endcode

    \note The parameter names are insignificant, but descriptive
    parameter names are recommended for the sake of readability.

    \section context Context

    Notice that commands are often context sensitive. While some command
    may accept an optional parameter that is filled up with the current
    target (channel/query) name when absent, another command may always
    inject the current target name as a certain parameter. Therefore
    IrcCommandParser must be kept up-to-date with the \ref target
    "current target" and the \ref channels "list of channels".

    \code
    // currently in a query, and also present on some channels
    parser->setTarget("jpnurmi");
    parser->setChannels(QStringList() << "#communi" << "#freenode");
    \endcode

    \section command-triggers Command triggers

    IrcCommandParser serves as a generic parser for typical IRC commands.
    It can be utilized for parsing commands from user input in GUI clients,
    and from messages from other clients when implementing IRC bots.

    The command parsing behavior is controlled by setting up command
    \ref triggers. Whilst a typical GUI client might use \c "/" as a command
    trigger, an IRC bot might use \c "!" and the nick name of the bot. The
    following snippet illustrates a typical GUI client usage.

    \code
    parser->setTarget("#communi");
    parser->setTriggers(QStringList() << "/");
    parser->parse(input);
    \endcode

    \p
    Input             | Result              | Description
    ------------------|---------------------|------------
    "hello"           | IrcCommand::Message | No matching command trigger => a message "hello" to \#communi
    "/join #channel"  | IrcCommand::Join    | Matching command trigger => a command to join "#channel"

    See the \ref bot "bot example" to see how the parser can be effectively utilized for IRC bots.

    \section parse-custom-commands Custom commands

    The parser also supports such custom client specific commands that
    are not sent to the server. Since IrcCommand does not know how to
    handle custom commands, the parser treats them as a special case
    injecting the command as a first parameter.

    \code
    IrcParser parser;
    parser.addCommand(IrcCommand::Custom, "QUERY <user>");
    IrcCommand* command = parser.parse("/query jpnurmi");
    Q_ASSERT(command->type() == IrcCommand::Custom);
    qDebug() << command->parameters(); // ("QUERY", "jpnurmi")
    \endcode
 */

/*!
    \enum IrcCommandParser::Detail
    This enum describes the available syntax details.
 */

/*!
    \var IrcCommandParser::Full
    \brief The syntax in full details
 */

/*!
    \var IrcCommandParser::NoTarget
    \brief The syntax has injected [target] removed
 */

/*!
    \var IrcCommandParser::NoPrefix
    \brief The syntax has \#channel prefixes removed
 */

/*!
    \var IrcCommandParser::NoEllipsis
    \brief The syntax has ellipsis... removed
 */

/*!
    \var IrcCommandParser::NoParentheses
    \brief The syntax has parentheses () removed
 */

/*!
    \var IrcCommandParser::NoBrackets
    \brief The syntax has brackets [] removed
 */

/*!
    \var IrcCommandParser::NoAngles
    \brief The syntax has angle brackets <> removed
 */

/*!
    \var IrcCommandParser::Visual
    \brief The syntax suitable for visual representation
 */

#ifndef IRC_DOXYGEN
IrcCommandParserPrivate::IrcCommandParserPrivate() : tolerant(false)
{
}

QList<IrcCommandInfo> IrcCommandParserPrivate::find(const QString& command) const
{
    QList<IrcCommandInfo> result;
    foreach (const IrcCommandInfo& cmd, commands) {
        if (cmd.command == command)
            result += cmd;
    }
    return result;
}

static inline bool isOptional(const QString& token)
{
    return token.startsWith(QLatin1Char('(')) && token.endsWith(QLatin1Char(')'));
}

static inline bool isMultiWord(const QString& token)
{
    return token.contains(QLatin1String("..."));
}

static inline bool isChannel(const QString& token)
{
    return token.contains(QLatin1Char('#'));
}

static inline bool isCurrent(const QString& token)
{
    return token.startsWith(QLatin1Char('[')) && token.endsWith(QLatin1Char(']'));
}

IrcCommandInfo IrcCommandParserPrivate::parseSyntax(IrcCommand::Type type, const QString& syntax)
{
    IrcCommandInfo cmd;
    QStringList tokens = syntax.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (!tokens.isEmpty()) {
        cmd.type = type;
        cmd.command = tokens.takeFirst().toUpper();
        cmd.syntax = tokens.join(QLatin1String(" "));
        cmd.max = tokens.count();

        IrcParameterInfo param;
        for (int i = 0; i < tokens.count(); ++i) {
            const QString& token = tokens.at(i);
            param.optional = isOptional(token);
            param.channel = isChannel(token);
            param.current = isCurrent(token);
            param.multi = isMultiWord(token);
            if (!param.optional)
                ++cmd.min;
            if (param.optional && param.channel)
                ++cmd.min;
            const bool last = (i == tokens.count() - 1);
            if (last && param.multi)
                cmd.max = INT_MAX;
            cmd.params += param;
        }
    }
    return cmd;
}

IrcCommand* IrcCommandParserPrivate::parseCommand(const IrcCommandInfo& command, const QString& input) const
{
    IrcCommand* cmd = 0;
    QStringList params;
    if (processParameters(command, input, &params)) {
        const int count = params.count();
        if (count >= command.min && count <= command.max) {
            cmd = new IrcCommand;
            cmd->setType(command.type);
            if (command.type == IrcCommand::Custom)
                params.prepend(command.command);
            cmd->setParameters(params);
        }
    }
    return cmd;
}

bool IrcCommandParserPrivate::processParameters(const IrcCommandInfo& command, const QString& input, QStringList* params) const
{
    IrcTokenizer tokenizer(input);
    for (int i = 0; i < command.params.count(); ++i) {
        const IrcParameterInfo& info = command.params.at(i);
        const IrcToken token = tokenizer.at(0);
        if (info.optional && info.channel) {
            if (onChannel()) {
                if (!token.isValid() || !channels.contains(token.text(), Qt::CaseInsensitive)) {
                    params->append(target);
                } else if (token.isValid()) {
                    tokenizer = tokenizer.mid(1);
                    params->append(token.text());
                }
            } else if (!channels.contains(token.text())) {
                return false;
            }
        } else if (info.current) {
            params->append(target);
        } else if (info.multi) {
            const QString multi = tokenizer.toString();
            if (!multi.isEmpty()) {
                params->append(multi);
                tokenizer.clear();
            }
        } else {
            tokenizer = tokenizer.mid(1);
            if (token.isValid())
                params->append(token.text());
        }
    }
    return tokenizer.isEmpty();
}

bool IrcCommandParserPrivate::processCommand(QString* input, int* removed) const
{
    foreach (const QString& trigger, triggers) {
        if (tolerant && trigger.length() == 1 && (input->startsWith(trigger.repeated(2)) || input->startsWith(trigger + QLatin1Char(' ')))) {
            // treat "//cmd" and "/ /cmd" as message (-> "/cmd")
            input->remove(0, 1);
            if (removed)
                *removed = 1;
            return false;
        } else if (input->startsWith(trigger)) {
            input->remove(0, trigger.length());
            if (removed)
                *removed = trigger.length();
            return true;
        }
    }
    return false;
}

bool IrcCommandParserPrivate::processMessage(QString* input, int* removed) const
{
    if (input->isEmpty())
        return false;
    if (triggers.isEmpty())
        return tolerant;
    if (processCommand(input, removed))
        return false;
    return tolerant;
}

bool IrcCommandParserPrivate::onChannel() const
{
    return channels.contains(target, Qt::CaseInsensitive);
}
#endif // IRC_DOXYGEN

/*!
    Constructs a command parser with \a parent.
 */
IrcCommandParser::IrcCommandParser(QObject* parent) : QObject(parent), d_ptr(new IrcCommandParserPrivate)
{
}

/*!
    Destructs the command parser.
 */
IrcCommandParser::~IrcCommandParser()
{
}

/*!
    This property holds the known commands.

    The commands are uppercased and in alphabetical order.

    \par Access function:
    \li QStringList <b>commands</b>() const

    \par Notifier signal:
    \li void <b>commandsChanged</b>(const QStringList& commands)

    \sa addCommand(), removeCommand()
 */
QStringList IrcCommandParser::commands() const
{
    Q_D(const IrcCommandParser);
    return d->commands.uniqueKeys();
}

/*!
    Returns syntax for the given \a command in given \a details level.
 */
QString IrcCommandParser::syntax(const QString& command, Details details) const
{
    Q_D(const IrcCommandParser);
    IrcCommandInfo info = d->find(command.toUpper()).value(0);
    if (!info.command.isEmpty()) {
        QString str = info.fullSyntax();
        if (details != Full) {
            if (details & NoTarget)
                str.remove(QRegExp("\\[[^\\]]+\\]"));
            if (details & NoPrefix)
                str.remove("#");
            if (details & NoEllipsis)
                str.remove("...");
            if (details & NoParentheses)
                str.remove("(").remove(")");
            if (details & NoBrackets)
                str.remove("[").remove("]");
            if (details & NoAngles)
                str.remove("<").remove(">");
        }
        return str.simplified();
    }
    return QString();
}

/*!
    Adds a command with \a type and \a syntax.
 */
void IrcCommandParser::addCommand(IrcCommand::Type type, const QString& syntax)
{
    Q_D(IrcCommandParser);
    IrcCommandInfo cmd = d->parseSyntax(type, syntax);
    if (!cmd.command.isEmpty()) {
        const bool contains = d->commands.contains(cmd.command);
        d->commands.insert(cmd.command, cmd);
        if (!contains)
            emit commandsChanged(commands());
    }
}

/*!
    Removes the command with \a type and \a syntax.
 */
void IrcCommandParser::removeCommand(IrcCommand::Type type, const QString& syntax)
{
    Q_D(IrcCommandParser);
    bool changed = false;
    QMutableMapIterator<QString, IrcCommandInfo> it(d->commands);
    while (it.hasNext()) {
        IrcCommandInfo cmd = it.next().value();
        if (cmd.type == type && (syntax.isEmpty() || !syntax.compare(cmd.fullSyntax(), Qt::CaseInsensitive))) {
            it.remove();
            if (!d->commands.contains(cmd.command))
                changed = true;
        }
    }
    if (changed)
        emit commandsChanged(commands());
}

/*!
    This property holds the available channels.

    \par Access functions:
    \li QStringList <b>channels</b>() const
    \li void <b>setChannels</b>(const QStringList& channels) [slot]

    \par Notifier signal:
    \li void <b>channelsChanged</b>(const QStringList& channels)

    \sa IrcBufferModel::channels()
 */
QStringList IrcCommandParser::channels() const
{
    Q_D(const IrcCommandParser);
    return d->channels;
}

void IrcCommandParser::setChannels(const QStringList& channels)
{
    Q_D(IrcCommandParser);
    if (d->channels != channels) {
        d->channels = channels;
        emit channelsChanged(channels);
    }
}

/*!
    This property holds the current target.

    \par Access functions:
    \li QString <b>target</b>() const
    \li void <b>setTarget</b>(const QString& target) [slot]

    \par Notifier signal:
    \li void <b>targetChanged</b>(const QString& target)
 */
QString IrcCommandParser::target() const
{
    Q_D(const IrcCommandParser);
    return d->target;
}

void IrcCommandParser::setTarget(const QString& target)
{
    Q_D(IrcCommandParser);
    if (d->target != target) {
        d->target = target;
        emit targetChanged(target);
    }
}

/*!
    This property holds the command triggers.

    \par Access functions:
    \li QStringList <b>triggers</b>() const
    \li void <b>setTriggers</b>(const QStringList& triggers) [slot]

    \par Notifier signal:
    \li void <b>triggersChanged</b>(const QStringList& triggers)
 */
QStringList IrcCommandParser::triggers() const
{
    Q_D(const IrcCommandParser);
    return d->triggers;
}

void IrcCommandParser::setTriggers(const QStringList& triggers)
{
    Q_D(IrcCommandParser);
    if (d->triggers != triggers) {
        d->triggers = triggers;
        emit triggersChanged(triggers);
    }
}

/*!
    \property bool IrcCommandParser::tolerant

    This property holds whether the parser is tolerant.

    A tolerant parser creates message commands out of input that does not
    start with a command trigger, and raw server commands when the input
    starts with a command trigger but the command is unrecognized. Known
    commands with invalid arguments are still considered invalid.

    The default value is \c false.

    \par Access functions:
    \li bool <b>isTolerant</b>() const
    \li void <b>setTolerant</b>(bool tolerant)

    \par Notifier signal:
    \li void <b>tolerancyChanged</b>(bool tolerant)

    \sa IrcCommand::Quote
 */
bool IrcCommandParser::isTolerant() const
{
    Q_D(const IrcCommandParser);
    return d->tolerant;
}

void IrcCommandParser::setTolerant(bool tolerant)
{
    Q_D(IrcCommandParser);
    if (d->tolerant != tolerant) {
        d->tolerant = tolerant;
        emit tolerancyChanged(tolerant);
    }
}

/*!
    Parses and returns the command for \a input, or \c 0 if the input is not valid.
 */
IrcCommand* IrcCommandParser::parse(const QString& input) const
{
    Q_D(const IrcCommandParser);
    QString message = input;
    if (d->processMessage(&message)) {
        return IrcCommand::createMessage(d->target, message.trimmed());
    } else if (!message.isEmpty()) {
        IrcTokenizer tokenizer(message);
        const QString command = tokenizer.at(0).text().toUpper();
        QString params = tokenizer.mid(1).toString();
        const QList<IrcCommandInfo> commands = d->find(command);
        if (!commands.isEmpty()) {
            foreach (const IrcCommandInfo& c, commands) {
                IrcCommand* cmd = d->parseCommand(c, params);
                if (cmd)
                    return cmd;
            }
        } else if (d->tolerant) {
            IrcCommandInfo custom = d->parseSyntax(IrcCommand::Quote, QString(QLatin1String("%1 (<parameters...>)")).arg(command));
            params.prepend(custom.command + QLatin1Char(' '));
            return d->parseCommand(custom, params);
        }
    }
    return 0;
}

/*!
    Clears the list of commands.

    \sa reset()
 */
void IrcCommandParser::clear()
{
    Q_D(IrcCommandParser);
    if (!d->commands.isEmpty()) {
        d->commands.clear();
        emit commandsChanged(QStringList());
    }
}

/*!
    Resets the channels and the current target.

    \sa clear()
 */
void IrcCommandParser::reset()
{
    setChannels(QStringList());
    setTarget(QString());
}

#include "moc_irccommandparser.cpp"

IRC_END_NAMESPACE
