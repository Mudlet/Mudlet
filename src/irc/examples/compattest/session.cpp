/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 *
 * $Id$
 */

#include "session.h"
#include <QtCore>

CompatIrcSession::CompatIrcSession(QObject* parent) : Irc::Session(parent)
{
    connect(this, SIGNAL(connected()), SLOT(on_connected()));
    connect(this, SIGNAL(disconnected()), SLOT(on_disconnected()));

    connect(this, SIGNAL(msgJoined(QString, QString)), SLOT(on_msgJoined(QString, QString)));
    connect(this, SIGNAL(msgParted(QString, QString, QString)), SLOT(on_msgParted(QString, QString, QString)));
    connect(this, SIGNAL(msgQuit(QString, QString)), SLOT(on_msgQuit(QString, QString)));
    connect(this, SIGNAL(msgNickChanged(QString, QString)), SLOT(on_msgNickChanged(QString, QString)));
    connect(this, SIGNAL(msgModeChanged(QString, QString, QString, QString)), SLOT(on_msgModeChanged(QString, QString, QString, QString)));
    connect(this, SIGNAL(msgTopicChanged(QString, QString, QString)), SLOT(on_msgTopicChanged(QString, QString, QString)));
    connect(this, SIGNAL(msgInvited(QString, QString, QString)), SLOT(on_msgInvited(QString, QString, QString)));
    connect(this, SIGNAL(msgKicked(QString, QString, QString, QString)), SLOT(on_msgKicked(QString, QString, QString, QString)));
    connect(this, SIGNAL(msgMessageReceived(QString, QString, QString)), SLOT(on_msgMessageReceived(QString, QString, QString)));
    connect(this, SIGNAL(msgNoticeReceived(QString, QString, QString)), SLOT(on_msgNoticeReceived(QString, QString, QString)));
    connect(this, SIGNAL(msgCtcpRequestReceived(QString, QString)), SLOT(on_msgCtcpRequestReceived(QString, QString)));
    connect(this, SIGNAL(msgCtcpReplyReceived(QString, QString)), SLOT(on_msgCtcpReplyReceived(QString, QString)));
    connect(this, SIGNAL(msgCtcpActionReceived(QString, QString, QString)), SLOT(on_msgCtcpActionReceived(QString, QString, QString)));
    connect(this, SIGNAL(msgNumericMessageReceived(QString, uint, QStringList)), SLOT(on_msgNumericMessageReceived(QString, uint, QStringList)));
    connect(this, SIGNAL(msgUnknownMessageReceived(QString, QStringList)), SLOT(on_msgUnknownMessageReceived(QString, QStringList)));
}

void CompatIrcSession::on_connected()
{
    qDebug() << "connected:";
}

void CompatIrcSession::on_disconnected()
{
    qDebug() << "disconnected:";
}

void CompatIrcSession::on_msgJoined(const QString& origin, const QString& channel)
{
    qDebug() << "join:" << origin << channel;
}

void CompatIrcSession::on_msgParted(const QString& origin, const QString& channel, const QString& message)
{
    qDebug() << "part:" << origin << channel << message;
}

void CompatIrcSession::on_msgQuit(const QString& origin, const QString& message)
{
    qDebug() << "quit:" << origin << message;
}

void CompatIrcSession::on_msgNickChanged(const QString& origin, const QString& nick)
{
    qDebug() << "nick:" << origin << nick;
}

void CompatIrcSession::on_msgModeChanged(const QString& origin, const QString& receiver, const QString& mode, const QString& args)
{
    qDebug() << "mode:" << origin << receiver << mode << args;
}

void CompatIrcSession::on_msgTopicChanged(const QString& origin, const QString& channel, const QString& topic)
{
    qDebug() << "topic:" << origin << channel << topic;
}

void CompatIrcSession::on_msgInvited(const QString& origin, const QString& receiver, const QString& channel)
{
    qDebug() << "invite:" << origin << receiver << channel;
}

void CompatIrcSession::on_msgKicked(const QString& origin, const QString& channel, const QString& nick, const QString& message)
{
    qDebug() << "kick:" << origin << channel << nick << message;
}

void CompatIrcSession::on_msgMessageReceived(const QString& origin, const QString& receiver, const QString& message)
{
    qDebug() << "message:" << origin << receiver << message;
}

void CompatIrcSession::on_msgNoticeReceived(const QString& origin, const QString& receiver, const QString& notice)
{
    qDebug() << "notice:" << origin << receiver << notice;
}

void CompatIrcSession::on_msgCtcpRequestReceived(const QString& origin, const QString& request)
{
    qDebug() << "ctcp request:" << origin << request;
}

void CompatIrcSession::on_msgCtcpReplyReceived(const QString& origin, const QString& reply)
{
    qDebug() << "ctcp reply:" << origin << reply;
}

void CompatIrcSession::on_msgCtcpActionReceived(const QString& origin, const QString& receiver, const QString& action)
{
    qDebug() << "ctcp action:" << origin << receiver << action;
}

void CompatIrcSession::on_msgNumericMessageReceived(const QString& origin, uint code, const QStringList& params)
{
    qDebug() << "numeric:" << origin << code << params;
}

void CompatIrcSession::on_msgUnknownMessageReceived(const QString& origin, const QStringList& params)
{
    qDebug() << "unknown:" << origin << params;
}
