/****************************************************************************
**                                MIT License
**
** Copyright (C) 2020-2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
** Author: Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
**
** This file is part of KDToolBox (https://github.com/KDAB/KDToolBox).
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, ** and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice (including the next paragraph)
** shall be included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF ** CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
****************************************************************************/

#ifndef KDTOOLBOX_SINGLESHOT_CONNECT_H
#define KDTOOLBOX_SINGLESHOT_CONNECT_H

#include <QObject>

#include <memory>

namespace KDToolBox {

template <typename Func1, typename Func2>
QMetaObject::Connection connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                          const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot,
                                          Qt::ConnectionType type = Qt::AutoConnection)
{
    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();

    auto singleShot =
            [=,
            connection = std::move(connection),
            receiver = const_cast<typename QtPrivate::FunctionPointer<Func2>::Object *>(receiver)]
                (auto && ... params)
    {
        QObject::disconnect(*connection);
        (receiver->*slot)(std::forward<decltype(params)>(params)...);
    };

    *connectionPtr = QObject::connect(sender, signal, receiver, std::move(singleShot), type);
    return *connectionPtr;
}

template <typename Func1, typename Func2>
typename std::enable_if<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0 &&
                        !QtPrivate::FunctionPointer<Func2>::IsPointerToMemberFunction, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                  const QObject *context, Func2 slot,
                  Qt::ConnectionType type = Qt::AutoConnection)
{
    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();
    auto singleShot =
            [=,
            slot = std::move(slot),
            connection = std::move(connection)]
                (auto && ... params) mutable
    {
        QObject::disconnect(*connection);
        slot(std::forward<decltype(params)>(params)...);
    };

    *connectionPtr = QObject::connect(sender, signal, context, std::move(singleShot), type);
    return *connectionPtr;
}

template <typename Func1, typename Func2>
typename std::enable_if<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
{
    return connectSingleShot(sender, signal, sender, std::move(slot), Qt::DirectConnection);
}

template <typename Func1, typename Func2>
typename std::enable_if<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                  const QObject *context, Func2 slot,
                  Qt::ConnectionType type = Qt::AutoConnection)
{
    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();
    auto singleShot =
            [=,
            slot = std::move(slot),
            connection = std::move(connection)]
                (auto && ... params) mutable
    {
        QObject::disconnect(*connection);
        slot(std::forward<decltype(params)>(params)...);
    };

    *connectionPtr = QObject::connect(sender, signal, context, std::move(singleShot), type);
    return *connectionPtr;
}

template <typename Func1, typename Func2>
typename std::enable_if<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
{
    return connectSingleShot(sender, signal, sender, std::move(slot), Qt::DirectConnection);
}

} // namespace KDToolBox

#endif // KDTOOLBOX_SINGLESHOT_CONNECT_H
