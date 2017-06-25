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

#ifndef IRCBUFFERMODEL_H
#define IRCBUFFERMODEL_H

#include <Irc>
#include <IrcGlobal>
#include <QtCore/qmetatype.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qabstractitemmodel.h>

IRC_BEGIN_NAMESPACE

class IrcBuffer;
class IrcChannel;
class IrcMessage;
class IrcNetwork;
class IrcConnection;
class IrcBufferModelPrivate;

class IRC_MODEL_EXPORT IrcBufferModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder)
    Q_PROPERTY(Irc::SortMethod sortMethod READ sortMethod WRITE setSortMethod)
    Q_PROPERTY(QStringList channels READ channels NOTIFY channelsChanged)
    Q_PROPERTY(Irc::DataRole displayRole READ displayRole WRITE setDisplayRole)
    Q_PROPERTY(bool persistent READ isPersistent WRITE setPersistent NOTIFY persistentChanged)
    Q_PROPERTY(QList<IrcBuffer*> buffers READ buffers NOTIFY buffersChanged)
    Q_PROPERTY(IrcConnection* connection READ connection WRITE setConnection NOTIFY connectionChanged)
    Q_PROPERTY(IrcNetwork* network READ network NOTIFY networkChanged)
    Q_PROPERTY(IrcBuffer* bufferPrototype READ bufferPrototype WRITE setBufferPrototype NOTIFY bufferPrototypeChanged)
    Q_PROPERTY(IrcChannel* channelPrototype READ channelPrototype WRITE setChannelPrototype NOTIFY channelPrototypeChanged)
    Q_PROPERTY(int joinDelay READ joinDelay WRITE setJoinDelay NOTIFY joinDelayChanged)
    Q_PROPERTY(bool monitorEnabled READ isMonitorEnabled WRITE setMonitorEnabled NOTIFY monitorEnabledChanged)

public:
    explicit IrcBufferModel(QObject* parent = 0);
    virtual ~IrcBufferModel();

    IrcConnection* connection() const;
    void setConnection(IrcConnection* connection);

    IrcNetwork* network() const;

    int count() const;
    bool isEmpty() const;
    QStringList channels() const;
    QList<IrcBuffer*> buffers() const;
    Q_INVOKABLE IrcBuffer* get(int index) const;
    Q_INVOKABLE IrcBuffer* find(const QString& title) const;
    Q_INVOKABLE bool contains(const QString& title) const;
    Q_INVOKABLE int indexOf(IrcBuffer* buffer) const;

    Q_INVOKABLE IrcBuffer* add(const QString& title);
    Q_INVOKABLE void add(IrcBuffer* buffer);
    Q_INVOKABLE void remove(const QString& title);
    Q_INVOKABLE void remove(IrcBuffer* buffer);

    Qt::SortOrder sortOrder() const;
    void setSortOrder(Qt::SortOrder order);

    Irc::SortMethod sortMethod() const;
    void setSortMethod(Irc::SortMethod method);

    Irc::DataRole displayRole() const;
    void setDisplayRole(Irc::DataRole role);

    bool isPersistent() const;
    void setPersistent(bool persistent);

    QModelIndex index(IrcBuffer* buffer) const;
    IrcBuffer* buffer(const QModelIndex& index) const;

    QHash<int, QByteArray> roleNames() const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;

    IrcBuffer* bufferPrototype() const;
    void setBufferPrototype(IrcBuffer* prototype);

    IrcChannel* channelPrototype() const;
    void setChannelPrototype(IrcChannel* prototype);

    int joinDelay() const;
    void setJoinDelay(int delay);

    bool isMonitorEnabled() const;
    void setMonitorEnabled(bool enabled);

    Q_INVOKABLE QByteArray saveState(int version = 0) const;
    Q_INVOKABLE bool restoreState(const QByteArray& state, int version = 0);

public Q_SLOTS:
    void clear();
    void receiveMessage(IrcMessage* message);
    void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder);
    void sort(Irc::SortMethod method, Qt::SortOrder order = Qt::AscendingOrder);

Q_SIGNALS:
    void countChanged(int count);
    void emptyChanged(bool empty);
    void added(IrcBuffer* buffer);
    void removed(IrcBuffer* buffer);
    void aboutToBeAdded(IrcBuffer* buffer);
    void aboutToBeRemoved(IrcBuffer* buffer);
    void persistentChanged(bool persistent);
    void buffersChanged(const QList<IrcBuffer*>& buffers);
    void channelsChanged(const QStringList& channels);
    void connectionChanged(IrcConnection* connection);
    void networkChanged(IrcNetwork* network);
    void messageIgnored(IrcMessage* message);
    void bufferPrototypeChanged(IrcBuffer* prototype);
    void channelPrototypeChanged(IrcChannel* prototype);
    void destroyed(IrcBufferModel* model);
    void joinDelayChanged(int delay);
    void monitorEnabledChanged(bool enabled);

protected Q_SLOTS:
    virtual IrcBuffer* createBuffer(const QString& title);
    virtual IrcChannel* createChannel(const QString& title);

protected:
    virtual bool lessThan(IrcBuffer* one, IrcBuffer* another, Irc::SortMethod method) const;

private:
    friend class IrcBufferLessThan;
    friend class IrcBufferGreaterThan;
    QScopedPointer<IrcBufferModelPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcBufferModel)
    Q_DISABLE_COPY(IrcBufferModel)

    Q_PRIVATE_SLOT(d_func(), void _irc_connected())
    Q_PRIVATE_SLOT(d_func(), void _irc_initialized())
    Q_PRIVATE_SLOT(d_func(), void _irc_disconnected())
    Q_PRIVATE_SLOT(d_func(), void _irc_bufferDestroyed(IrcBuffer*))
    Q_PRIVATE_SLOT(d_func(), void _irc_restoreBuffers())
    Q_PRIVATE_SLOT(d_func(), void _irc_monitorStatus())
};

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcBufferModel*))

#endif // IRCBUFFERMODEL_H
