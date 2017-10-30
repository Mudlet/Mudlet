/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmeida.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "ircmessageformatter.h"

#include "mudlet.h"

// CHECK: should IrcTextFormat be inside the MSVC guards
#include "pre_guard.h"
#include <IrcTextFormat>
#include <QLocale>
#include <QStringList>
#include "post_guard.h"

QString IrcMessageFormatter::formatMessage(IrcMessage* message, bool isForLua)
{
    QString formatted;
    QString color = QLatin1String("#f29010");

    IrcNumericMessage* nMsg;

    switch (message->type()) {
    case IrcMessage::Away:
        formatted = formatAwayMessage(static_cast<IrcAwayMessage*>(message), isForLua);
        break;
    case IrcMessage::Invite:
        formatted = formatInviteMessage(static_cast<IrcInviteMessage*>(message), isForLua);
        break;
    case IrcMessage::Join:
        formatted = formatJoinMessage(static_cast<IrcJoinMessage*>(message), isForLua);
        break;
    case IrcMessage::Mode:
        formatted = formatModeMessage(static_cast<IrcModeMessage*>(message), isForLua);
        break;
    case IrcMessage::Motd:
        formatted = formatMotdMessage(static_cast<IrcMotdMessage*>(message), isForLua);
        break;
    case IrcMessage::Names:
        formatted = formatNamesMessage(static_cast<IrcNamesMessage*>(message), isForLua);
        break;
    case IrcMessage::Nick:
        formatted = formatNickMessage(static_cast<IrcNickMessage*>(message), isForLua);
        break;
    case IrcMessage::Notice:
        formatted = formatNoticeMessage(static_cast<IrcNoticeMessage*>(message), isForLua);
        break;
    case IrcMessage::Part:
        formatted = formatPartMessage(static_cast<IrcPartMessage*>(message), isForLua);
        break;
    case IrcMessage::Pong:
        formatted = formatPongMessage(static_cast<IrcPongMessage*>(message), isForLua);
        break;
    case IrcMessage::Private:
        formatted = formatPrivateMessage(static_cast<IrcPrivateMessage*>(message), isForLua);
        break;
    case IrcMessage::Quit:
        formatted = formatQuitMessage(static_cast<IrcQuitMessage*>(message), isForLua);
        break;
    case IrcMessage::Topic:
        formatted = formatTopicMessage(static_cast<IrcTopicMessage*>(message), isForLua);
        color = QLatin1String("#3283bc");
        break;
    case IrcMessage::Whois:
        formatted = formatWhoisMessage(static_cast<IrcWhoisMessage*>(message), isForLua);
        break;
    case IrcMessage::Whowas:
        formatted = formatWhowasMessage(static_cast<IrcWhowasMessage*>(message), isForLua);
        break;
    case IrcMessage::WhoReply:
        formatted = formatWhoReplyMessage(static_cast<IrcWhoReplyMessage*>(message), isForLua);
        break;
    case IrcMessage::Error:
        formatted = formatErrorMessage(static_cast<IrcErrorMessage*>(message), isForLua);
        color = QLatin1String("indianred");
        break;
    case IrcMessage::Unknown:
        formatted = formatUnknownMessage(message, isForLua);
        break;
    case IrcMessage::Numeric:
        nMsg = static_cast<IrcNumericMessage*>(message);
        formatted = formatNumericMessage(nMsg, isForLua);
        // if you change this, change formatErrorMessage too
        if (Irc::codeToString(nMsg->code()).startsWith(QLatin1String("ERR_"))) {
            color = QLatin1String("indianred");
        }
        break;
    default:
        break;
    }
    return formatMessage(formatted, color, isForLua);
}

QString IrcMessageFormatter::formatMessage(const QString& message, QString color, bool isForLua)
{
    if (!message.isEmpty()) {
        if (isForLua) { // lua has no need for the timestamp or HTML added here.
            return message;
        }

        QString formatted = QStringLiteral("[%1] %2").arg(QTime::currentTime().toString(), message);

        if (message.startsWith(QLatin1String("!"))) {
            formatted = QStringLiteral("<font color='gray'>%1</font>").arg(formatted);
        } else if (message.startsWith(QLatin1String("*"))) {
            formatted = QStringLiteral("<font color='maroon'>%1</font>").arg(formatted);
        } else if (message.startsWith(QLatin1String("$"))) {
            formatted = QStringLiteral("<font color='#3cc46e'>%1</font>").arg(formatted);
        } else if (message.startsWith(QLatin1String("["))) {
            formatted = QStringLiteral("<font color='%2'>%1</font>").arg(formatted, (color.isEmpty() ? QLatin1String("#f29010") : color ));
        }
        return formatted;
    }
    return QString();
}

QString IrcMessageFormatter::formatAwayMessage(IrcAwayMessage* message, bool isForLua)
{
    QString content;
    if (isForLua) {
        content = IrcTextFormat().toPlainText(message->content());
    } else {
        content = IrcTextFormat().toHtml(message->content());
    }

    if (message->flags() & IrcMessage::Own) {
        return QStringLiteral("! %1").arg(content);
    } else if (!message->content().isEmpty()) {
        if (isForLua) {
            return QStringLiteral("! %1 is away (%2)").arg(message->nick(), content);
        } else {
            return tr("! %1 is away (%2)", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), content);
        }
    }

    if (isForLua) {
        return QStringLiteral("! %1 is back").arg(message->nick());
    } else {
        return tr("! %1 is back", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick());
    }
}

QString IrcMessageFormatter::formatInviteMessage(IrcInviteMessage* message, bool isForLua)
{
    if (message->isReply()) {
        if (isForLua) {
            return QStringLiteral("! invited %1 to %2").arg(message->user(), message->channel());
        } else {
            return tr("! invited %1 to %2", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->user(), message->channel());
        }
    } else {
        if (isForLua) {
            return QStringLiteral("! %1 invited to %2").arg(message->nick(), message->channel());
        } else {
            return tr("! %1 invited to %2", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->channel());
        }
    }
}

QString IrcMessageFormatter::formatJoinMessage(IrcJoinMessage* message, bool isForLua)
{
    if (message->flags() & IrcMessage::Own) {
        if (isForLua) {
            return QStringLiteral("! You have joined %1 as %2").arg(message->channel(), message->nick());
        } else {
            return tr("! You have joined %1 as %2", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->channel(), message->nick());
        }
    } else {
        if (isForLua) {
            return QStringLiteral("! %1 has joined %2").arg(message->nick(), message->channel());
        } else {
            return tr("! %1 has joined %2", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->channel());
        }
    }
}

QString IrcMessageFormatter::formatKickMessage(IrcKickMessage* message, bool isForLua)
{
    if (isForLua) {
        return QStringLiteral("! %1 kicked %2").arg(message->nick(), message->user());
    } else {
        return tr("! %1 kicked %2", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->user());
    }
}

QString IrcMessageFormatter::formatModeMessage(IrcModeMessage* message, bool isForLua)
{
    QString args = message->arguments().join(QChar::Space);
    if (message->isReply()) {
        if (isForLua) {
            return QStringLiteral("! %1 mode is %2 %3").arg(message->target(), message->mode(), args);
        } else {
            return tr("! %1 mode is %2 %3","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->target(), message->mode(), args);
        }
    } else {
        if (isForLua) {
            return QStringLiteral("! %1 sets mode %2 %3 %4").arg(message->nick(), message->target(), message->mode(), args);
        } else {
            return tr("! %1 sets mode %2 %3 %4","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->target(), message->mode(), args);
        }
    }
}

QString IrcMessageFormatter::formatMotdMessage(IrcMotdMessage* message, bool isForLua)
{
    QString motdData;
    foreach (const QString& line, message->lines()) {
        if (isForLua) {
            motdData += QStringLiteral("[MOTD] %1\n").arg(IrcTextFormat().toPlainText(line));
        } else {
            // <br /> was being used but that is XML, NOT valid HTML...!
            // Using a tr(...) as an argument to a QStringLiteral(...) to hide
            // HTML from translators...
            motdData += QStringLiteral("%1<br>\n")
                    .arg(tr("[MOTD] %1", "MOTD is an initialism for 'Message of the Day' and may need to be replaced with a language specific replacement.")
                         .arg(IrcTextFormat().toHtml(line)));
        }
    }
    return motdData;
}

QString IrcMessageFormatter::formatNamesMessage(IrcNamesMessage* message, bool isForLua)
{
    const QString count = QString::number(message->names().count());
    if (isForLua) {
        // lua actually needs the names for parsing, since getting a names
        // list from the UI userModel alone would be limiting to the IRC commands.
        return QStringLiteral("! %1 has %2 users: %3").arg(message->channel(), count, message->names().join(QChar::Space));
    } else {
        return tr("! %1 has %n users","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!", message->names().count())
                .arg(message->channel(), count);
    }
}

QString IrcMessageFormatter::formatNickMessage(IrcNickMessage* message, bool isForLua)
{
    return tr("! %1 has changed nick to %2", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->oldNick(), message->newNick());
}

QString IrcMessageFormatter::formatNoticeMessage(IrcNoticeMessage* message, bool isForLua)
{
    if (message->isReply()) {
        const QStringList params = message->content().split(QChar::Space, QString::SkipEmptyParts);
        const QString cmd = params.value(0);
        if (!cmd.compare(QLatin1String("PING"), Qt::CaseInsensitive)) {
            const QString secs = formatSeconds(params.value(1).toInt());
            if (isForLua) {
                return QStringLiteral("! %1 replied in %2").arg(message->nick(), secs);
            } else {
                return tr("! %1 replied in %2","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), secs);
            }
        } else if (!cmd.compare(QLatin1String("TIME"), Qt::CaseInsensitive)) {
            const QString rest = QStringList(params.mid(1)).join(QChar::Space);
            if (isForLua) {
                return QStringLiteral("! %1 time is %2").arg(message->nick(), rest);
            } else {
                return tr("! %1 time is %2","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), rest);
            }
        } else if (!cmd.compare(QLatin1String("VERSION"), Qt::CaseInsensitive)) {
            const QString rest = QStringList(params.mid(1)).join(QChar::Space);
            if (isForLua) {
                return QStringLiteral("! %1 version is %2").arg(message->nick(), rest);
            } else {
                return tr("! %1 version is %2","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), rest);
            }
        }
    }

    QString pfx = message->statusPrefix();
    if (!pfx.isEmpty()) {
        pfx.prepend(QLatin1String(":"));
    }

    if (message->isPrivate()) {
        if (isForLua) {
            return QStringLiteral("[%1%2] %3").arg(message->nick(), pfx,IrcTextFormat().toPlainText(message->content()));
        } else {
            return tr("[%1%2] %3", "%1 is nickname, %2 is a status prefix, %3 is message.").arg(message->nick(), pfx, IrcTextFormat().toHtml(message->content()));
        }
    } else {
        if (isForLua) {
            // lua only needs the message text.
            return IrcTextFormat().toPlainText(message->content());
        } else {
            return tr("&lt;%1%2&gt; [%3] %4", "%1 is nickname, %2 is a status prefix, %3 is target of message, %4 is message; please preserve HTML entities (the '%lt;' and '%gt;' that get converted to '<' and '>' and NOT the markers around an HTML tag).")
                    .arg(message->nick(), pfx, message->target(), IrcTextFormat().toHtml(message->content()));
        }
    }
}

QString IrcMessageFormatter::formatNumericMessage(IrcNumericMessage* message, bool isForLua)
{
    // CHECK: Why 300?
    if (message->code() < 300) {
        const QString info = QStringList(message->parameters().mid(1)).join(QChar::Space);
        if (isForLua) {
            return QStringLiteral("[INFO] %1").arg(IrcTextFormat().toPlainText(info));
        } else {
            return tr("[INFO] %1").arg(IrcTextFormat().toHtml(info));
        }
    }

    switch (message->code()) {
    case Irc::RPL_VERSION:
        if (isForLua) {
            return QStringLiteral("! %1 version is %2").arg(message->nick(), message->parameters().value(1));
        } else {
            return tr("! %1 version is %2","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->parameters().value(1));
        }
        // break is not needed as both branches lead to a return...!

    case Irc::RPL_TIME:
        if (isForLua) {
            return QStringLiteral("! %1 time is %2").arg(message->parameters().value(1), message->parameters().value(2));
        } else {
            return tr("! %1 time is %2","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->parameters().value(1), message->parameters().value(2));
        }
        // break is not needed as both branches lead to a return...!

    default:
        break;
    }

    if (message->isComposed() || message->flags() & IrcMessage::Implicit) {
        return QString();
    }

    const QString info = QStringList(message->parameters().mid(1)).join(QChar::Space);

    // if you change this, change formatErrorMessage too
    if (Irc::codeToString(message->code()).startsWith(QLatin1String("ERR_"))) {
        if (isForLua) {
            return QStringLiteral("[ERROR] %1").arg(IrcTextFormat().toPlainText(info));
        } else {
            //: This is not going through cTelnet::postMessage(...) !
            return tr("[ERROR] %1").arg(IrcTextFormat().toHtml(info));
        }
    } else  if (message->code() == Irc::RPL_CHANNEL_URL) {
        if (isForLua) {
            return QStringLiteral("[Channel URL] %1").arg(IrcTextFormat().toPlainText(info));
        } else {
            return tr("[Channel URL] %1").arg(IrcTextFormat().toHtml(info));
        }
    } else {
        if (isForLua) {
            return QStringLiteral("[%1] %2").arg(QString::number(message->code()), IrcTextFormat().toPlainText(info));
        } else {
            return tr("[%1] %2", "Not obvious that any translation is actually required for this").arg(QString::number(message->code()), IrcTextFormat().toHtml(info));
        }
    }
}

QString IrcMessageFormatter::formatErrorMessage(IrcErrorMessage* message, bool isForLua)
{
    // if you change this, change ERR_ in formatNumericMessage too
    if (isForLua) {
        return QStringLiteral("[ERROR] %1").arg(message->error());
    } else {
        // This does not go through cTelnet::postMessage(...) so does not
        // need a '-' after the "[ERROR] "!
        return tr("[ERROR] %1").arg(message->error());
    }
}

QString IrcMessageFormatter::formatPartMessage(IrcPartMessage* message, bool isForLua)
{
    if (message->reason().isEmpty()) {
        if (isForLua) {
            return QStringLiteral("! %1 has left %2").arg(message->nick(), message->channel());
        } else {
            return tr("! %1 has left %2","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->channel());
        }
    } else {
        if (isForLua) {
            return QStringLiteral("! %1 has left %2 (%3)").arg(message->nick(), message->channel(), message->reason());
        } else {
            return tr("! %1 has left %2 (%3)","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick(), message->channel(), message->reason());
        }
    }
}

QString IrcMessageFormatter::formatPongMessage(IrcPongMessage* message, bool isForLua)
{
    quint64 msec = message->timeStamp().toMSecsSinceEpoch();
    quint64 dms = (QDateTime::currentMSecsSinceEpoch() - msec);
    // QString::sprintf(...) was/is OBSOLETE...!
    if (isForLua) {
        return QStringLiteral("! %1 replied in %n seconds").arg(message->nick(), QString::number(dms / 1000.0, 'f', 3));
    } else {
        return tr("! %1 replied in %2 seconds","The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!; the seconds value (%2) is a decimal value not an integer one and will be formatted as per specified locale, which makes handling pluralization moot...")
                .arg(message->nick(), QLocale(mudlet::self()->getGuiLanguage()).toString(static_cast<double>(dms / 1000.0), 'f', 3));
    }
}

// Normal messages sent to channels are processed by our client as if they are private messages.
QString IrcMessageFormatter::formatPrivateMessage(IrcPrivateMessage* message, bool isForLua)
{
    QString content;
    if (isForLua) {
        content = IrcTextFormat().toPlainText(message->content());
    } else {
        content = IrcTextFormat().toHtml(message->content());
    }

    if (message->isAction()) {
        if (isForLua) {
            return QStringLiteral("* %1 %2").arg(message->nick(), content);
        } else {
            return tr("* %1 %2","").arg(message->nick(), content);
        }
    } else {
        if (isForLua) {
            // lua only needs the message text here.  Nick and target are sent as arguments to postIrcMessage()
            return content;
        } else {
            return tr("<b>&lt;%1&gt;</b> %2").arg(message->nick(), content);
        }
    }
}

QString IrcMessageFormatter::formatQuitMessage(IrcQuitMessage* message, bool isForLua)
{
    if (message->reason().isEmpty()) {
        if (isForLua) {
            return QStringLiteral("! %1 has quit").arg(message->nick());
        } else {
            return tr("! %1 has quit").arg(message->nick());
        }
    } else {
        if (isForLua) {
            return QStringLiteral("! %1 has quit (%2)").arg(message->nick(), message->reason());
        } else {
            return tr("! %1 has quit (%2)").arg(message->nick(), message->reason());
        }
    }
}

QString IrcMessageFormatter::formatTopicMessage(IrcTopicMessage* message, bool isForLua)
{
    if (message->isReply()) {
        if (message->topic().isEmpty()) {
            if (isForLua) {
                return QLatin1String("! no topic");
            } else {
                return tr("! no topic", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!");
            }
        } else {
            if (isForLua) {
                return QStringLiteral("[TOPIC] %1").arg(IrcTextFormat().toPlainText(message->topic()));
            } else {
                return tr("[TOPIC] %1", "Not sure how the expression [TOPIC] should be translated").arg(IrcTextFormat().toHtml(message->topic()));
            }
        }
    } else {
        if (message->topic().isEmpty()) {
            if (isForLua) {
                return QStringLiteral("! %1 cleared topic").arg(message->nick());
            } else {
                return tr("! %1 cleared topic", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick());
            }
        } else {
            if (isForLua) {
                return QStringLiteral("! %1 changed topic").arg(message->nick());
            } else {
                return tr("! %1 changed topic", "The leading ! is a special status marker used programmatically - not an actual exclamination mark - leave unchanged!").arg(message->nick());
            }
        }
    }
}

QString IrcMessageFormatter::formatUnknownMessage(IrcMessage* message, bool isForLua)
{
    if (isForLua) {
        return QStringLiteral("? %1 %2 %3").arg(message->nick(), message->command(), message->parameters().join(QChar::Space));
    } else {
        return tr("? %1 %2 %3", "The leading ? is a special status marker used programmatically - not an actual question mark - leave unchanged!").arg(message->nick(), message->command(), message->parameters().join(QChar::Space));
    }
}

QString IrcMessageFormatter::formatWhoisMessage(IrcWhoisMessage* message, bool isForLua)
{
    if (isForLua) {
        // CHECK: Are linefeeds not required here?
        // Can only have up to 10 arguments in (earlier ?) Qt 5.x versions
        QString result = QStringLiteral("[WHOIS] %1 is %2@%3 (%4)"
                                        "[WHOIS] %1 is connected via %5 (%6)")
                .arg(message->nick(), message->ident(), message->host(), message->realName(),
                     message->server(), message->info());
        result.append(QStringLiteral("[WHOIS] %1 is connected since %2 (idle %3)%4%5%6%7%8")
                      .arg(message->nick(), message->since().toString(), formatDuration(message->idle()),
                           (message->awayReason().isEmpty() ? QString() : QStringLiteral("[WHOIS] %1 is away: %2").arg(message->nick(), message->awayReason())),
                           (message->account().isEmpty() ? QString() : QStringLiteral("[WHOIS] %1 is logged in as %2").arg(message->nick(), message->account())),
                           (message->address().isEmpty() ? QString() : QStringLiteral("[WHOIS] %1 is connected from %2").arg(message->nick(), message->address())),
                           (message->isSecure() ? QStringLiteral("[WHOIS] %1 is using a secure connection").arg(message->nick()) : QString()),
                           (message->channels().isEmpty() ? QString() : QStringLiteral("[WHOIS] %1 is on %2").arg(message->nick(), message->channels().join(QChar::Space)))));
        return result;
    } else {
        // CHECK: Are linefeeds not required here?
        QString result = tr("[WHOIS] %1 is %2@%3 (%4)n"
                            "[WHOIS] %1 is connected via %5 (%6)n")
                .arg(message->nick(), message->ident(), message->host(), message->realName(),
                     message->server(), message->info());

        result.append(tr("[WHOIS] %1 is connected since %7 (idle %8)%9%10%11%12%13")
                      .arg(message->nick(), message->since().toString(), formatDuration(message->idle()),
                           (message->awayReason().isEmpty() ? QString() : tr("[WHOIS] %1 is away: %2").arg(message->nick(), message->awayReason())),
                           (message->address().isEmpty() ? QString() : tr("[WHOIS] %1 is connected from %2").arg(message->nick(), message->address())),
                           (message->account().isEmpty() ? QString() : tr("[WHOIS] %1 is logged in as %2").arg(message->nick(), message->account())),
                           (message->isSecure() ? tr("[WHOIS] %1 is using a secure connection").arg(message->nick()) : QString()),
                           (message->channels().isEmpty() ? QString() : tr("[WHOIS] %1 is on %2").arg(message->nick(), message->channels().join(QChar::Space)))));
        return result;
    }
}

QString IrcMessageFormatter::formatWhowasMessage(IrcWhowasMessage* message, bool isForLua)
{
    if (isForLua) {
        // CHECK: Are linefeeds not required here?
        return QStringLiteral("[WHOWAS] %1 was %2@%3 (%4)"
                              "[WHOWAS] %1 was connected via %5 (%6)%7")
                .arg(message->nick(),
                     message->ident(),
                     message->host(),
                     message->realName(),
                     message->server(),
                     message->info(),
                     ( message->account().isEmpty() ? QString() : QStringLiteral("\n[WHOWAS] %1 was logged in as %2")
                                                      .arg(message->nick(), message->account())));
    } else {
        // CHECK: Are linefeeds not required here?
        return tr("[WHOWAS] %1 was %2@%3 (%4)"
                  "[WHOWAS] %1 was connected via %5 (%6)%7")
                .arg(message->nick(),
                     message->ident(),
                     message->host(),
                     message->realName(),
                     message->server(),
                     message->info(),
                     ( message->account().isEmpty() ? QString() : tr("[WHOWAS] %1 was logged in as %2")
                                                      .arg(message->nick(), message->account())));
    }
}

QString IrcMessageFormatter::formatWhoReplyMessage(IrcWhoReplyMessage* message, bool isForLua)
{
    if (isForLua) {
        return QStringLiteral("[WHO] %1 (%2) %3 %4").arg(message->nick(), message->realName(), (message->isAway() ? QLatin1String(" - away") : QString()), ( message->isServOp() ? QLatin1String(" - server operator") : QString()));
    } else {
        return tr("[WHO] %1 (%2) %3 %4").arg(message->nick(), message->realName(), (message->isAway() ? tr(" - away") : QString()), ( message->isServOp() ? tr(" - server operator") : QString()));
    }
}

QString IrcMessageFormatter::formatSeconds(int secs)
{
    return tr("%1s").arg(QDateTime::fromTime_t(secs).secsTo(QDateTime::currentDateTime()));
}

QString IrcMessageFormatter::formatDuration(int secs)
{
    QStringList idle;
    if (int days = secs / 86400) {
        idle += tr("%n days","",days).arg(days);
    }
    secs %= 86400;
    if (int hours = secs / 3600) {
        idle += tr("%n hours","",hours).arg(hours);
    }
    secs %= 3600;
    if (int mins = secs / 60) {
        idle += tr("%n minutes","",mins).arg(mins);
    }
    idle += tr("%n seconds","",secs % 60).arg(secs % 60);
    return idle.join(QChar::Space);
}
