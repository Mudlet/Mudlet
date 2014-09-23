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

#include <QtQuick>
#include <IrcSession>
#include <IrcCommand>
#include <IrcMessage>
#include <IrcSender>
#include <Irc>

class QuickIrcSender : public QObject
{
    Q_OBJECT

public:
    explicit QuickIrcSender(QObject* parent = 0) : QObject(parent) { }

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
QML_DECLARE_TYPE(QuickIrcSender)

class CommuniPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Communi.QQmlExtensionInterface")

public:
    virtual void initializeEngine(QQmlEngine* engine, const char* uri)
    {
        QQmlExtensionPlugin::initializeEngine(engine, uri);

        QQmlContext* context = engine->rootContext();
        context->setContextProperty("IrcSender", new QuickIrcSender(context));
    }

    void registerTypes(const char *uri)
    {
        qmlRegisterType<IrcSession>(uri, 2, 0, "IrcSession");
        qmlRegisterType<IrcCommand>(uri, 2, 0, "IrcCommand");
        qmlRegisterType<IrcMessage>(uri, 2, 0, "IrcMessage");

        qmlRegisterUncreatableType<Irc>(uri, 2, 0, "Irc", "");
    }
};

#include "plugin.moc"
