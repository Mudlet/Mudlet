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

#include <QtDeclarative>

#include <IrcCore>
#include <IrcModel>
#include <IrcUtil>

IRC_BEGIN_NAMESPACE

class IrcQmlFilter : public QObject,
                     public IrcCommandFilter,
                     public IrcMessageFilter
{
    Q_OBJECT
    Q_INTERFACES(IrcCommandFilter IrcMessageFilter)
    Q_PROPERTY(IrcConnection* connection READ connection WRITE setConnection NOTIFY connectionChanged)

public:
    IrcQmlFilter(QObject* parent = 0) : QObject(parent), conn(0) { }

    IrcConnection* connection() const { return conn; }
    void setConnection(IrcConnection* connection)
    {
        if (conn != connection) {
            if (conn) {
                conn->removeCommandFilter(this);
                conn->removeMessageFilter(this);
            }
            conn = connection;
            if (conn) {
                conn->installCommandFilter(this);
                conn->installMessageFilter(this);
            }
            emit connectionChanged();
        }
    }

    bool commandFilter(IrcCommand* cmd)
    {
        // QML: QVariant commandFilter(QVariant)
        const QMetaObject* mo = metaObject();
        int idx = mo->indexOfMethod("commandFilter(QVariant)");
        if (idx != -1) {
            QVariant ret;
            QMetaMethod method = mo->method(idx);
            method.invoke(this, Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QVariant::fromValue(cmd)));
            return ret.toBool();
        }
        return false;
    }

    bool messageFilter(IrcMessage* msg)
    {
        // QML: QVariant messageFilter(QVariant)
        const QMetaObject* mo = metaObject();
        int idx = mo->indexOfMethod("messageFilter(QVariant)");
        if (idx != -1) {
            QVariant ret;
            QMetaMethod method = mo->method(idx);
            method.invoke(this, Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QVariant::fromValue(msg)));
            return ret.toBool();
        }
        return false;
    }

signals:
    void connectionChanged();

private:
    QPointer<IrcConnection> conn;
};

class CommuniPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char* uri) {
        // IrcCore
        Irc::registerMetaTypes();
        qmlRegisterType<Irc>(uri, 3, 0, "Irc");
        qmlRegisterType<Irc>(uri, 3, 2, "Irc");
        qmlRegisterType<Irc>(uri, 3, 3, "Irc");
        qmlRegisterType<Irc>(uri, 3, 4, "Irc");
        qmlRegisterType<Irc>(uri, 3, 5, "Irc");
        qmlRegisterType<IrcCommand>(uri, 3, 0, "IrcCommand");
        qmlRegisterType<IrcConnection>(uri, 3, 0, "IrcConnection");
        qmlRegisterUncreatableType<IrcMessage>(uri, 3, 0, "IrcMessage", "Cannot create an instance of IrcMessage. Use IrcConnection::messageReceived() signal instead.");
        qmlRegisterUncreatableType<IrcNetwork>(uri, 3, 0, "IrcNetwork", "Cannot create an instance of IrcNetwork. Use IrcConnection::network property instead.");
        qmlRegisterType<IrcQmlFilter>(uri, 3, 0, "IrcMessageFilter");
        qmlRegisterType<IrcQmlFilter>(uri, 3, 0, "IrcCommandFilter");
        qmlRegisterType<IrcQmlFilter>(uri, 3, 0, "IrcFilter");

        // IrcModel
        qmlRegisterType<IrcBuffer>(uri, 3, 0, "IrcBuffer");
        qmlRegisterType<IrcBufferModel>(uri, 3, 0, "IrcBufferModel");
        qmlRegisterType<IrcChannel>(uri, 3, 0, "IrcChannel");
        qmlRegisterType<IrcUser>(uri, 3, 0, "IrcUser");
        qmlRegisterType<IrcUserModel>(uri, 3, 0, "IrcUserModel");

        // IrcUtil
        qmlRegisterType<IrcCommandParser>(uri, 3, 0, "IrcCommandParser");
        qmlRegisterType<IrcLagTimer>(uri, 3, 0, "IrcLagTimer");
        qmlRegisterType<IrcTextFormat>(uri, 3, 0, "IrcTextFormat");
        qmlRegisterUncreatableType<IrcPalette>(uri, 3, 0, "IrcPalette", "Cannot create an instance of IrcPalette. Use IrcTextFormat::palette property instead.");
        qmlRegisterType<IrcCompleter>(uri, 3, 1, "IrcCompleter");
    }
};

IRC_END_NAMESPACE

#include "plugin.moc"

Q_EXPORT_PLUGIN2(communiplugin, CommuniPlugin);
