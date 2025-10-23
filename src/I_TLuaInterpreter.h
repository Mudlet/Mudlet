#pragma once

class I_TLuaInterpreter {
public:
    virtual ~I_TLuaInterpreter() = default;

    virtual bool compile(const QString &script, QString &error, const QString &context) = 0;
    virtual bool call(const QString &funcName, const QString &scriptName, const bool muteDebugOutput = false) = 0;
    virtual bool callEventHandler(const QString &scriptName, const TEvent &event) = 0;
};
