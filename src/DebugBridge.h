#ifndef DEBUGBRIDGE_H
#define DEBUGBRIDGE_H

#include <memory>
#include <QString>
#include "I_TDebug.h"

class DebugBridge
{
public:
    using FactoryFunc = std::unique_ptr<I_TDebug>(*)(const QString&, const QString&);

    static void registerFactory(FactoryFunc f) { s_factory = f; }

    static std::unique_ptr<I_TDebug> create(const QString& fg, const QString& bg) {
        if (s_factory)
            return s_factory(fg, bg);
        return nullptr;
    }

private:
    static FactoryFunc s_factory;
};

#endif
