/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#include <QtDeclarative>
#include <IrcSession>
#include <IrcCommand>
#include <IrcMessage>
#include <IrcSender>
#include <Irc>

class DeclarativeIrcSender : public QObject
{
    Q_OBJECT

public:
    explicit DeclarativeIrcSender(QObject* parent = 0) : QObject(parent) { }

    Q_INVOKABLE static QString name(const IrcSender& sender)
    {
        return sender.name();
    }

    Q_INVOKABLE static QString user(const IrcSender& sender)
    {
        return sender.user();
    }

    Q_INVOKABLE static QString host(const IrcSender& sender)
    {
        return sender.host();
    }
};

QML_DECLARE_TYPE(Irc)
QML_DECLARE_TYPE(IrcCommand)
QML_DECLARE_TYPE(IrcMessage)
QML_DECLARE_TYPE(IrcSession)
QML_DECLARE_TYPE(DeclarativeIrcSender)

class CommuniPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    virtual void initializeEngine(QDeclarativeEngine* engine, const char* uri)
    {
        QDeclarativeExtensionPlugin::initializeEngine(engine, uri);

        QDeclarativeContext* context = engine->rootContext();
        context->setContextProperty("IrcSender", new DeclarativeIrcSender(context));
    }

    void registerTypes(const char *uri)
    {
        qmlRegisterType<IrcSession>(uri, 1, 0, "IrcSession");
        qmlRegisterType<IrcCommand>(uri, 1, 0, "IrcCommand");
        qmlRegisterType<IrcMessage>(uri, 1, 0, "IrcMessage");

        qmlRegisterUncreatableType<Irc>(uri, 1, 0, "Irc", "");
    }
};

#include "plugin.moc"

Q_EXPORT_PLUGIN2(communiplugin, CommuniPlugin);
