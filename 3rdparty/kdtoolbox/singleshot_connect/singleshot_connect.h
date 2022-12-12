/****************************************************************************
**                                MIT License
**
** Copyright (C) 2020-2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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
#include <tuple>
#include <functional>

namespace KDToolBox {

#if __cplusplus >= 201703L
namespace Internal {

template<std::size_t... I, typename... Args>
static inline auto makeTruncatedArgsImpl(std::index_sequence<I...>, Args &&...args)
{
    [[maybe_unused]] auto tempTuple = std::forward_as_tuple(std::forward<Args>(args)...);
    return std::forward_as_tuple(std::get<I>(std::move(tempTuple))...);
}

// Take args... as input and truncate it ArgsCount
template<std::size_t ArgsCount, typename... Args>
static inline auto makeTruncatedArgs(Args &&...args)
{
    return makeTruncatedArgsImpl(std::make_index_sequence<ArgsCount>(), std::forward<Args>(args)...);
}

} // namespace Internal
#endif // __cplusplus >= 201703L

template <typename Func1, typename Func2>
QMetaObject::Connection connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                          const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot,
                                          Qt::ConnectionType type = Qt::AutoConnection)
{
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    typedef QtPrivate::FunctionPointer<Func2> SlotType;
    static_assert(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                        "The slot requires more arguments than the signal provides.");
    static_assert((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                        "Signal and slot arguments are not compatible.");
    static_assert((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                        "Return type of the slot is not compatible with the return type of the signal.");

    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();

    auto singleShot =
            [=,
            connection = std::move(connection),
            receiver = const_cast<typename QtPrivate::FunctionPointer<Func2>::Object *>(receiver)]
                (auto && ... params)
    {
        QObject::disconnect(*connection);


#if __cplusplus >= 201703L
        constexpr std::size_t SlotArgumentCount = SlotType::ArgumentCount;
        std::apply(slot, std::tuple_cat(std::tuple(receiver), Internal::makeTruncatedArgs<SlotArgumentCount>(std::forward<decltype(params)>(params)...)));
#else
        (receiver->*slot)(std::forward<decltype(params)>(params)...);
#endif
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
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    constexpr int FunctorArgumentCount =
        QtPrivate::ComputeFunctorArgumentCount<Func2, typename SignalType::Arguments>::Value;

    static_assert((FunctorArgumentCount >= 0), "Signal and slot arguments are not compatible.");

    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();
    auto singleShot =
            [=,
            slot = std::move(slot),
            connection = std::move(connection)]
                (auto && ... params) mutable
    {
        QObject::disconnect(*connection);
#if __cplusplus >= 201703L
        // MSVC fails to compile if we try to reuse FunctorArgumentCount...
        constexpr int SlotArgumentCount =
            QtPrivate::ComputeFunctorArgumentCount<Func2, typename SignalType::Arguments>::Value;
        std::apply(slot, Internal::makeTruncatedArgs<SlotArgumentCount>(std::forward<decltype(params)>(params)...));
#else
        slot(std::forward<decltype(params)>(params)...);
#endif
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
