#pragma once

#include "TEvent.h"

class I_TLuaInterpreter {
public:
    virtual ~I_TLuaInterpreter() = default;

    virtual bool _compile(const QString &script, QString &error, const QString &context) = 0;
    virtual bool _call(const QString &funcName, const QString &scriptName, const bool muteDebugOutput = false) = 0;
    virtual bool _callEventHandler(const QString &scriptName, const TEvent &event) = 0;
};
