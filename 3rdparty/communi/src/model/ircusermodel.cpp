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

#include "ircusermodel.h"
#include "ircusermodel_p.h"
#include "ircbuffermodel.h"
#include "ircconnection.h"
#include "ircchannel_p.h"
#include "ircuser.h"
#include <qpointer.h>

IRC_BEGIN_NAMESPACE

/*!
    \file ircusermodel.h
    \brief \#include &lt;IrcUserModel&gt;
 */

/*!
    \class IrcUserModel ircusermodel.h <IrcUserModel>
    \ingroup models
    \brief Keeps track of channel users.

    In order to keep track of channel users, create an instance of IrcUserModel.
    It will notify via signals when users are added and/or removed. IrcUserModel
    can be used directly as a data model for Qt's item views - both in C++ and QML.

    \code
    void ChatView::setChannel(IrcChannel* channel)
    {
        IrcUserModel* model = new IrcUserModel(channel);
        connect(model, SIGNAL(added(IrcUser*)), this, SLOT(onUserAdded(IrcUser*)));
        connect(model, SIGNAL(removed(IrcUser*)), this, SLOT(onUserRemoved(IrcUser*)));
        nickCompleter->setModel(model);
        userListView->setModel(model);
    }
    \endcode
*/

/*!
    \fn void IrcUserModel::added(IrcUser* user)

    This signal is emitted when a \a user is added to the list of users.
 */

/*!
    \fn void IrcUserModel::removed(IrcUser* user)

    This signal is emitted when a \a user is removed from the list of users.
 */

/*!
    \fn void IrcUserModel::aboutToBeAdded(IrcUser* user)

    This signal is emitted just before a \a user is added to the list of users.
 */

/*!
    \fn void IrcUserModel::aboutToBeRemoved(IrcUser* user)

    This signal is emitted just before a \a user is removed from the list of users.
 */

#ifndef IRC_DOXYGEN
class IrcUserLessThan
{
public:
    IrcUserLessThan(IrcUserModel* model, Irc::SortMethod method) : model(model), method(method) { }
    bool operator()(IrcUser* u1, IrcUser* u2) const { return model->lessThan(u1, u2, method); }
private:
    IrcUserModel* model;
    Irc::SortMethod method;
};

class IrcUserGreaterThan
{
public:
    IrcUserGreaterThan(IrcUserModel* model, Irc::SortMethod method) : model(model), method(method) { }
    bool operator()(IrcUser* u1, IrcUser* u2) const { return model->lessThan(u2, u1, method); }
private:
    IrcUserModel* model;
    Irc::SortMethod method;
};

IrcUserModelPrivate::IrcUserModelPrivate() : q_ptr(0), role(Irc::TitleRole),
    sortMethod(Irc::SortByHand), sortOrder(Qt::AscendingOrder)
{
}

void IrcUserModelPrivate::addUser(IrcUser* user, bool notify)
{
    insertUser(-1, user, notify);
}

void IrcUserModelPrivate::insertUser(int index, IrcUser* user, bool notify)
{
    Q_Q(IrcUserModel);
    if (index == -1)
        index = userList.count();
    if (sortMethod != Irc::SortByHand) {
        QList<IrcUser*>::iterator it;
        if (sortOrder == Qt::AscendingOrder)
            it = qUpperBound(userList.begin(), userList.end(), user, IrcUserLessThan(q, sortMethod));
        else
            it = qUpperBound(userList.begin(), userList.end(), user, IrcUserGreaterThan(q, sortMethod));
        index = it - userList.begin();
    }
    if (notify)
        emit q->aboutToBeAdded(user);
    q->beginInsertRows(QModelIndex(), index, index);
    userList.insert(index, user);
    updateTitles();
    q->endInsertRows();
    if (notify) {
        emit q->added(user);
        emit q->namesChanged(IrcChannelPrivate::get(channel)->names);
        emit q->titlesChanged(titles);
        emit q->usersChanged(userList);
        emit q->countChanged(userList.count());
        if (userList.count() == 1)
            emit q->emptyChanged(false);
    }
}

void IrcUserModelPrivate::removeUser(IrcUser* user, bool notify)
{
    Q_Q(IrcUserModel);
    int idx = userList.indexOf(user);
    if (idx != -1) {
        if (notify)
            emit q->aboutToBeRemoved(user);
        q->beginRemoveRows(QModelIndex(), idx, idx);
        userList.removeAt(idx);
        updateTitles();
        q->endRemoveRows();
        if (notify) {
            emit q->removed(user);
            emit q->namesChanged(IrcChannelPrivate::get(channel)->names);
            emit q->titlesChanged(titles);
            emit q->usersChanged(userList);
            emit q->countChanged(userList.count());
            if (userList.isEmpty())
                emit q->emptyChanged(true);
        }
    }
}

void IrcUserModelPrivate::setUsers(const QList<IrcUser*>& users, bool reset)
{
    Q_Q(IrcUserModel);
    bool wasEmpty = userList.isEmpty();
    if (reset)
        q->beginResetModel();
    userList = users;
    if (sortMethod != Irc::SortByHand) {
        if (sortOrder == Qt::AscendingOrder)
            qSort(userList.begin(), userList.end(), IrcUserLessThan(q, sortMethod));
        else
            qSort(userList.begin(), userList.end(), IrcUserGreaterThan(q, sortMethod));
    }
    updateTitles();
    if (reset)
        q->endResetModel();
    QStringList names;
    if (channel)
        names = IrcChannelPrivate::get(channel)->names;
    emit q->namesChanged(names);
    emit q->titlesChanged(titles);
    emit q->usersChanged(userList);
    emit q->countChanged(userList.count());
    if (wasEmpty != userList.isEmpty())
        emit q->emptyChanged(userList.isEmpty());
}

void IrcUserModelPrivate::renameUser(IrcUser* user)
{
    Q_Q(IrcUserModel);
    if (updateUser(user) && sortMethod != Irc::SortByHand) {
        QList<IrcUser*> users = userList;
        const bool notify = false;
        removeUser(user, notify);
        insertUser(-1, user, notify);
        if (updateTitles())
            emit q->titlesChanged(titles);
        if (users != userList)
            emit q->usersChanged(userList);
    }
}

void IrcUserModelPrivate::setUserMode(IrcUser* user)
{
    Q_Q(IrcUserModel);
    if (updateUser(user) && sortMethod == Irc::SortByTitle) {
        const bool notify = false;
        removeUser(user, notify);
        insertUser(0, user, notify);
        if (updateTitles())
            emit q->titlesChanged(titles);
        emit q->usersChanged(userList);
    }
}

void IrcUserModelPrivate::promoteUser(IrcUser* user)
{
    Q_Q(IrcUserModel);
    if (sortMethod == Irc::SortByActivity) {
        const bool notify = false;
        removeUser(user, notify);
        insertUser(0, user, notify);
        if (updateTitles())
            emit q->titlesChanged(titles);
        emit q->usersChanged(userList);
    }
}

bool IrcUserModelPrivate::updateUser(IrcUser* user)
{
    Q_Q(IrcUserModel);
    const int idx = userList.indexOf(user);
    if (idx != -1) {
        QModelIndex index = q->index(idx, 0);
        emit q->dataChanged(index, index);
        return true;
    }
    return false;
}

bool IrcUserModelPrivate::updateTitles()
{
    QStringList prev = titles;
    titles.clear();
    foreach (IrcUser* user, userList)
        titles += user->title();
    return titles != prev;
}

#endif // IRC_DOXYGEN

/*!
    Constructs a new model with \a parent.

    \note If \a parent is an instance of IrcChannel, it will be
    automatically assigned to \ref IrcUserModel::channel "channel".
 */
IrcUserModel::IrcUserModel(QObject* parent) : QAbstractListModel(parent), d_ptr(new IrcUserModelPrivate)
{
    Q_D(IrcUserModel);
    d->q_ptr = this;
    setChannel(qobject_cast<IrcChannel*>(parent));

    qRegisterMetaType<IrcUser*>();
    qRegisterMetaType<QList<IrcUser*> >();
}

/*!
    Destructs the model.
 */
IrcUserModel::~IrcUserModel()
{
    Q_D(IrcUserModel);
    if (d->channel)
        IrcChannelPrivate::get(d->channel)->userModels.removeOne(this);
}

/*!
    This property holds the channel.

    \par Access functions:
    \li \ref IrcChannel* <b>channel</b>() const
    \li void <b>setChannel</b>(\ref IrcChannel* channel)

    \par Notifier signal:
    \li void <b>channelChanged</b>(\ref IrcChannel* channel)
 */
IrcChannel* IrcUserModel::channel() const
{
    Q_D(const IrcUserModel);
    return d->channel;
}

void IrcUserModel::setChannel(IrcChannel* channel)
{
    Q_D(IrcUserModel);
    if (d->channel != channel) {
        beginResetModel();
        if (d->channel)
            IrcChannelPrivate::get(d->channel)->userModels.removeOne(this);

        d->channel = channel;

        QList<IrcUser*> users;
        if (d->channel) {
            IrcChannelPrivate::get(d->channel)->userModels.append(this);
            if (d->sortMethod == Irc::SortByActivity)
                users = IrcChannelPrivate::get(d->channel)->activeUsers;
            else
                users = IrcChannelPrivate::get(d->channel)->userList;
        }
        const bool reset = false;
        d->setUsers(users, reset);
        endResetModel();

        emit channelChanged(channel);
    }
}

/*!
    This property holds the number of users on the channel.

    \par Access function:
    \li int <b>count</b>() const

    \par Notifier signal:
    \li void <b>countChanged</b>(int count)
 */
int IrcUserModel::count() const
{
    return rowCount();
}

/*!
    \since 3.1
    \property bool IrcUserModel::empty

    This property holds the whether the model is empty.

    \par Access function:
    \li bool <b>isEmpty</b>() const

    \par Notifier signal:
    \li void <b>emptyChanged</b>(bool empty)
 */
bool IrcUserModel::isEmpty() const
{
    Q_D(const IrcUserModel);
    return d->userList.isEmpty();
}

/*!
    This property holds the list of names in alphabetical order.

    \par Access function:
    \li QStringList <b>names</b>() const

    \par Notifier signal:
    \li void <b>namesChanged</b>(const QStringList& names)
 */
QStringList IrcUserModel::names() const
{
    Q_D(const IrcUserModel);
    if (d->channel && !d->userList.isEmpty())
        return IrcChannelPrivate::get(d->channel)->names;
    return QStringList();
}

/*!
    \since 3.3

    This property holds the list of titles.

    \par Access function:
    \li QStringList <b>titles</b>() const

    \par Notifier signal:
    \li void <b>titlesChanged</b>(const QStringList& titles)
 */
QStringList IrcUserModel::titles() const
{
    Q_D(const IrcUserModel);
    return d->titles;
}

/*!
    This property holds the list of users.

    The order of users is kept as sent from the server.

    \par Access function:
    \li QList<\ref IrcUser*> <b>users</b>() const

    \par Notifier signal:
    \li void <b>usersChanged</b>(const QList<\ref IrcUser*>& users)
 */
QList<IrcUser*> IrcUserModel::users() const
{
    Q_D(const IrcUserModel);
    return d->userList;
}

/*!
    Returns the user object at \a index.
 */
IrcUser* IrcUserModel::get(int index) const
{
    Q_D(const IrcUserModel);
    return d->userList.value(index);
}

/*!
    Returns the user object for \a name or \c 0 if not found.
 */
IrcUser* IrcUserModel::find(const QString& name) const
{
    Q_D(const IrcUserModel);
    if (d->channel && !d->userList.isEmpty())
        return IrcChannelPrivate::get(d->channel)->userMap.value(name);
    return 0;
}

/*!
    Returns \c true if the model contains \a name.
 */
bool IrcUserModel::contains(const QString& name) const
{
    Q_D(const IrcUserModel);
    if (d->channel && !d->userList.isEmpty())
        return IrcChannelPrivate::get(d->channel)->userMap.contains(name);
    return false;
}

/*!
    Returns the index of the specified \a user,
    or \c -1 if the model does not contain the \a user.
 */
int IrcUserModel::indexOf(IrcUser* user) const
{
    Q_D(const IrcUserModel);
    return d->userList.indexOf(user);
}

/*!
    This property holds the model sort method.

    The default value is \c Irc::SortByHand.

    Method              | Description                                                                                       | Example
    --------------------|---------------------------------------------------------------------------------------------------|----------------------------------------------
    Irc::SortByHand     | Users are not sorted automatically, but only by calling sort().                                   | -
    Irc::SortByName     | Users are sorted alphabetically, ignoring any mode prefix.                                        | "bot", "@ChanServ", "jpnurmi", "+qtassistant"
    Irc::SortByTitle    | Users are sorted alphabetically, and special users (operators, voiced users) before normal users. | "@ChanServ", "+qtassistant", "bot", "jpnurmi"
    Irc::SortByActivity | Users are sorted based on their activity, last active and mentioned (1) users first.              | -

    1) For performance reasons, IrcUserModel does \b not scan the whole channel
       messages to find out if a channel user was mentioned. IrcUserModel merely
       checks if channel messages \b begin with the name of a user in the model.

    \par Access functions:
    \li Irc::SortMethod <b>sortMethod</b>() const
    \li void <b>setSortMethod</b>(Irc::SortMethod method)

    \sa sort(), lessThan()
 */
Irc::SortMethod IrcUserModel::sortMethod() const
{
    Q_D(const IrcUserModel);
    return d->sortMethod;
}

void IrcUserModel::setSortMethod(Irc::SortMethod method)
{
    Q_D(IrcUserModel);
    if (d->sortMethod != method) {
        d->sortMethod = method;
        if (method == Irc::SortByActivity && d->channel) {
            d->userList = IrcChannelPrivate::get(d->channel)->activeUsers;
            if (d->updateTitles())
                emit titlesChanged(d->titles);
        }
        if (d->sortMethod != Irc::SortByHand && !d->userList.isEmpty())
            sort(d->sortMethod, d->sortOrder);
    }
}

/*!
    This property holds the model sort order.

    The default value is \c Qt::AscendingOrder.

    \par Access functions:
    \li Qt::SortOrder <b>sortOrder</b>() const
    \li void <b>setSortOrder</b>(Qt::SortOrder order)

    \sa sort(), lessThan()
 */
Qt::SortOrder IrcUserModel::sortOrder() const
{
    Q_D(const IrcUserModel);
    return d->sortOrder;
}

void IrcUserModel::setSortOrder(Qt::SortOrder order)
{
    Q_D(IrcUserModel);
    if (d->sortOrder != order) {
        d->sortOrder = order;
        if (d->sortMethod != Irc::SortByHand && !d->userList.isEmpty())
            sort(d->sortMethod, d->sortOrder);
    }
}

/*!
    This property holds the display role.

    The specified data role is returned for Qt::DisplayRole.

    The default value is \ref Irc::TitleRole.

    \par Access functions:
    \li \ref Irc::DataRole <b>displayRole</b>() const
    \li void <b>setDisplayRole</b>(\ref Irc::DataRole role)
 */
Irc::DataRole IrcUserModel::displayRole() const
{
    Q_D(const IrcUserModel);
    return d->role;
}

void IrcUserModel::setDisplayRole(Irc::DataRole role)
{
    Q_D(IrcUserModel);
    d->role = role;
}

/*!
    Returns the model index for \a user.
 */
QModelIndex IrcUserModel::index(IrcUser* user) const
{
    Q_D(const IrcUserModel);
    return index(d->userList.indexOf(user));
}

/*!
    Returns the user for model \a index.
 */
IrcUser* IrcUserModel::user(const QModelIndex& index) const
{
    if (!hasIndex(index.row(), index.column()))
        return 0;

    return static_cast<IrcUser*>(index.internalPointer());
}

/*!
    The following role names are provided by default:

    Role            | Name      | Type     | Example
    --------------- | ----------|----------|--------
    Qt::DisplayRole | "display" | 1)       | -
    Irc::UserRole   | "user"    | IrcUser* | &lt;object&gt;
    Irc::NameRole   | "name"    | QString  | "jpnurmi"
    Irc::PrefixRole | "prefix"  | QString  | "@"
    Irc::ModeRole   | "mode"    | QString  | "o"
    Irc::TitleRole  | "title"   | QString  | "@jpnurmi"

    1) The type depends on \ref displayRole.
 */
QHash<int, QByteArray> IrcUserModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[Irc::UserRole] = "user";
    roles[Irc::NameRole] = "name";
    roles[Irc::PrefixRole] = "prefix";
    roles[Irc::ModeRole] = "mode";
    roles[Irc::TitleRole] = "title";
    return roles;
}

/*!
    Returns the number of users on the channel.
 */
int IrcUserModel::rowCount(const QModelIndex& parent) const
{
    Q_D(const IrcUserModel);
    if (parent.isValid() || !d->channel)
        return 0;

    return d->userList.count();
}

/*!
    Returns the data for specified \a role referred to by the \a index.

    \sa Irc::DataRole, roleNames()
 */
QVariant IrcUserModel::data(const QModelIndex& index, int role) const
{
    Q_D(const IrcUserModel);
    if (!d->channel || !hasIndex(index.row(), index.column(), index.parent()))
        return QVariant();

    IrcUser* user = static_cast<IrcUser*>(index.internalPointer());
    Q_ASSERT(user);

    switch (role) {
    case Qt::DisplayRole:
        return data(index, d->role);
    case Irc::UserRole:
        return QVariant::fromValue(user);
    case Irc::NameRole:
        return user->name();
    case Irc::PrefixRole:
        return user->prefix().left(1);
    case Irc::ModeRole:
        return user->mode().left(1);
    case Irc::TitleRole:
        return user->title();
    }

    return QVariant();
}

/*!
    Returns the index of the item in the model specified by the given \a row, \a column and \a parent index.
 */
QModelIndex IrcUserModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_D(const IrcUserModel);
    if (!d->channel || !hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, d->userList.value(row));
}

/*!
    Clears the model.
 */
void IrcUserModel::clear()
{
    Q_D(IrcUserModel);
    if (!d->userList.isEmpty()) {
        beginResetModel();
        d->userList.clear();
        endResetModel();
        emit namesChanged(QStringList());
        emit titlesChanged(QStringList());
        emit usersChanged(QList<IrcUser*>());
        emit countChanged(0);
        emit emptyChanged(true);
    }
}

/*!
    Sorts the model using the given \a order.
 */
void IrcUserModel::sort(int column, Qt::SortOrder order)
{
    Q_D(IrcUserModel);
    if (column == 0)
        sort(d->sortMethod, order);
}

/*!
    Sorts the model using the given \a method and \a order.

    \sa lessThan()
 */
void IrcUserModel::sort(Irc::SortMethod method, Qt::SortOrder order)
{
    Q_D(IrcUserModel);
    if (method == Irc::SortByHand)
        return;

    emit layoutAboutToBeChanged();

    QList<IrcUser*> persistentUsers;
    QModelIndexList oldPersistentIndexes = persistentIndexList();
    foreach (const QModelIndex& index, oldPersistentIndexes)
        persistentUsers += static_cast<IrcUser*>(index.internalPointer());

    if (order == Qt::AscendingOrder)
        qSort(d->userList.begin(), d->userList.end(), IrcUserLessThan(this, method));
    else
        qSort(d->userList.begin(), d->userList.end(), IrcUserGreaterThan(this, method));

    if (d->updateTitles())
        emit titlesChanged(d->titles);

    QModelIndexList newPersistentIndexes;
    foreach (IrcUser* user, persistentUsers)
        newPersistentIndexes += index(d->userList.indexOf(user));
    changePersistentIndexList(oldPersistentIndexes, newPersistentIndexes);

    emit layoutChanged();
}

/*!
    Returns \c true if \a one buffer is "less than" \a another,
    otherwise returns \c false.

    The default implementation sorts according to the specified sort method.
    Reimplement this function in order to customize the sort order.

    \sa sort(), sortMethod
 */
bool IrcUserModel::lessThan(IrcUser* one, IrcUser* another, Irc::SortMethod method) const
{
    if (method == Irc::SortByActivity) {
        QList<IrcUser*> activeUsers = IrcChannelPrivate::get(one->channel())->activeUsers;
        const int i1 = activeUsers.indexOf(one);
        const int i2 = activeUsers.indexOf(another);
        return i1 < i2;
    } else if (method == Irc::SortByTitle) {
        const IrcNetwork* network = one->channel()->network();
        const QStringList prefixes = network->prefixes();

        const QString p1 = one->prefix();
        const QString p2 = another->prefix();

        const int i1 = !p1.isEmpty() ? prefixes.indexOf(p1.at(0)) : -1;
        const int i2 = !p2.isEmpty() ? prefixes.indexOf(p2.at(0)) : -1;

        if (i1 >= 0 && i2 < 0)
            return true;
        if (i1 < 0 && i2 >= 0)
            return false;
        if (i1 >= 0 && i2 >= 0 && i1 != i2)
            return i1 < i2;
    }

    // Irc::SortByName
    const QString n1 = one->name();
    const QString n2 = another->name();
    return n1.compare(n2, Qt::CaseInsensitive) < 0;
}

#include "moc_ircusermodel.cpp"

IRC_END_NAMESPACE
