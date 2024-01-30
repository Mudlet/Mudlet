/*
  This file is part of KDToolBox.

  SPDX-FileCopyrightText: 2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>

  SPDX-License-Identifier: MIT
*/

#ifndef KDTOOLBOX_SINGLESHOT_CONNECT_H
#define KDTOOLBOX_SINGLESHOT_CONNECT_H

#include <QObject>

#include <functional>
#include <memory>
#include <tuple>

namespace KDToolBox
{

#if __cplusplus >= 201703L
namespace Internal
{

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

// This could be a lambda, but it doesn't work on MSVC,
// so here's an "unrolled" implementation.
template<int ArgumentCount, typename SlotType>
struct SingleShotStruct
{
    SlotType slot;
    std::unique_ptr<QMetaObject::Connection> connection;

    template<typename... T>
    void operator()(T &&...args)
    {
        QObject::disconnect(*connection);
#if __cplusplus >= 201703L
        std::apply(slot, makeTruncatedArgs<ArgumentCount>(std::forward<T>(args)...));
#else
        slot(std::forward<T>(args)...);
#endif
    }
};

template<int ArgumentCount, typename SlotType>
auto makeSingleShotStruct(SlotType &&slot, std::unique_ptr<QMetaObject::Connection> &&connection)
{
    return SingleShotStruct<ArgumentCount, SlotType>{std::forward<SlotType>(slot), std::move(connection)};
}

} // namespace Internal
#endif // __cplusplus >= 201703L

template<typename Func1, typename Func2>
QMetaObject::Connection connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender,
                                          Func1 signal,
                                          const typename QtPrivate::FunctionPointer<Func2>::Object *receiver,
                                          Func2 slot, Qt::ConnectionType type = Qt::AutoConnection)
{
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    typedef QtPrivate::FunctionPointer<Func2> SlotType;
    static_assert(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                  "The slot requires more arguments than the signal provides.");
    static_assert(
        (QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
        "Signal and slot arguments are not compatible.");
    static_assert(
        (QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
        "Return type of the slot is not compatible with the return type of the signal.");

    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();

    auto singleShot =
        [=, connection = std::move(connection),
         receiver = const_cast<typename QtPrivate::FunctionPointer<Func2>::Object *>(receiver)](auto &&...params) {
            QObject::disconnect(*connection);

#if __cplusplus >= 201703L
            constexpr std::size_t SlotArgumentCount = SlotType::ArgumentCount;
            std::apply(slot, std::tuple_cat(std::tuple(receiver), Internal::makeTruncatedArgs<SlotArgumentCount>(
                                                                      std::forward<decltype(params)>(params)...)));
#else
            (receiver->*slot)(std::forward<decltype(params)>(params)...);
#endif
        };

    *connectionPtr = QObject::connect(sender, signal, receiver, std::move(singleShot), type);
    return *connectionPtr;
}

template<typename Func1, typename Func2>
typename std::enable_if<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0 &&
                            !QtPrivate::FunctionPointer<Func2>::IsPointerToMemberFunction,
                        QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                  const QObject *context, Func2 slot, Qt::ConnectionType type = Qt::AutoConnection)
{
    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();
    auto singleShot = [slot = std::move(slot), connection = std::move(connection)](auto &&...params) mutable {
        QObject::disconnect(*connection);
        slot(std::forward<decltype(params)>(params)...);
    };

    *connectionPtr = QObject::connect(sender, signal, context, std::move(singleShot), type);
    return *connectionPtr;
}

template<typename Func1, typename Func2>
typename std::enable_if<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
{
    return connectSingleShot(sender, signal, sender, std::move(slot), Qt::DirectConnection);
}

template<typename Func1, typename Func2>
typename std::enable_if<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                  const QObject *context, Func2 slot, Qt::ConnectionType type = Qt::AutoConnection)
{
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    constexpr int FunctorArgumentCount =
        QtPrivate::ComputeFunctorArgumentCount<Func2, typename SignalType::Arguments>::Value;

    static_assert((FunctorArgumentCount >= 0), "Signal and slot arguments are not compatible.");

    auto connection = std::make_unique<QMetaObject::Connection>();
    auto connectionPtr = connection.get();

    auto singleShot = Internal::makeSingleShotStruct<FunctorArgumentCount>(std::move(slot), std::move(connection));

    *connectionPtr = QObject::connect(sender, signal, context, std::move(singleShot), type);
    return *connectionPtr;
}

template<typename Func1, typename Func2>
typename std::enable_if<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::type
connectSingleShot(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
{
    return connectSingleShot(sender, signal, sender, std::move(slot), Qt::DirectConnection);
}

} // namespace KDToolBox

#endif // KDTOOLBOX_SINGLESHOT_CONNECT_H
