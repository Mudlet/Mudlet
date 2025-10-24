#pragma once

#include "TScript.h"
#include "ScriptUnit.h"
#include "I_TLuaInterpreter.h"

class I_Host
{
public:
    virtual ~I_Host() = default;

    virtual void _registerEventHandler(const QString &handler, TScript *script) = 0;
    virtual void _unregisterEventHandler(const QString &handler, TScript *script) = 0;
    virtual ScriptUnit* _getScriptUnit() = 0;
    virtual I_TLuaInterpreter* _getLuaInterpreter() = 0;
    virtual void _dlgTriggerEditor_cleanReset() = 0;           

    virtual bool _getResetProfile() = 0;        //get Host::mResetProfile
    virtual bool _getBlockScript() = 0;         //get Host::mBlockScriptCompile
    virtual QMap<QString, QMap<QString, QString>>* _getModuleInfo() = 0;          //get Host::mModuleInfo
    virtual bool _getMudletSmDebugMode() = 0;    //get mudlet::smDebugMode
    virtual QMap<QString, QList<TScript*>>* _getEventHandlerMap() = 0; //get Host::mEventHandlerMap

};