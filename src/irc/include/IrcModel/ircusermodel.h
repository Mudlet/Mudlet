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

#ifndef IRCUSERMODEL_H
#define IRCUSERMODEL_H

#include <Irc>
#include <IrcGlobal>
#include <QtCore/qmetatype.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qabstractitemmodel.h>

IRC_BEGIN_NAMESPACE

class IrcUser;
class IrcChannel;
class IrcMessage;
class IrcUserModelPrivate;

class IRC_MODEL_EXPORT IrcUserModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)
    Q_PROPERTY(QStringList names READ names NOTIFY namesChanged)
    Q_PROPERTY(QStringList titles READ titles NOTIFY titlesChanged)
    Q_PROPERTY(QList<IrcUser*> users READ users NOTIFY usersChanged)
    Q_PROPERTY(Irc::DataRole displayRole READ displayRole WRITE setDisplayRole)
    Q_PROPERTY(IrcChannel* channel READ channel WRITE setChannel NOTIFY channelChanged)
    Q_PROPERTY(Irc::SortMethod sortMethod READ sortMethod WRITE setSortMethod)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder)

public:
    explicit IrcUserModel(QObject* parent = 0);
    virtual ~IrcUserModel();

    IrcChannel* channel() const;
    void setChannel(IrcChannel* channel);

    int count() const;
    bool isEmpty() const;
    QStringList names() const;
    QStringList titles() const;
    QList<IrcUser*> users() const;
    Q_INVOKABLE IrcUser* get(int index) const;
    Q_INVOKABLE IrcUser* find(const QString& name) const;
    Q_INVOKABLE bool contains(const QString& name) const;
    Q_INVOKABLE int indexOf(IrcUser* user) const;

    Irc::DataRole displayRole() const;
    void setDisplayRole(Irc::DataRole role);

    Irc::SortMethod sortMethod() const;
    void setSortMethod(Irc::SortMethod method);

    Qt::SortOrder sortOrder() const;
    void setSortOrder(Qt::SortOrder order);

    QModelIndex index(IrcUser* user) const;
    IrcUser* user(const QModelIndex& index) const;

    QHash<int, QByteArray> roleNames() const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;

public Q_SLOTS:
    void clear();
    void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder);
    void sort(Irc::SortMethod method, Qt::SortOrder order = Qt::AscendingOrder);

Q_SIGNALS:
    void added(IrcUser* user);
    void removed(IrcUser* user);
    void aboutToBeAdded(IrcUser* user);
    void aboutToBeRemoved(IrcUser* user);
    void countChanged(int count);
    void emptyChanged(bool empty);
    void namesChanged(const QStringList& names);
    void titlesChanged(const QStringList& titles);
    void usersChanged(const QList<IrcUser*>& users);
    void channelChanged(IrcChannel* channel);

protected:
    virtual bool lessThan(IrcUser* one, IrcUser* another, Irc::SortMethod method) const;

private:
    friend class IrcUserLessThan;
    friend class IrcChannelPrivate;
    friend class IrcUserGreaterThan;
    QScopedPointer<IrcUserModelPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcUserModel)
    Q_DISABLE_COPY(IrcUserModel)
};

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcUserModel*))

#endif // IRCUSERMODEL_H
