/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "TScript.h"


#include "Host.h"
#include "TDebug.h"
#include "mudlet.h"

TScript::TScript( TScript * parent, Host * pHost )
: Tree<TScript>( parent )
, exportItem(true)
, mModuleMasterFolder(false)
, mpHost(pHost)
, mNeedsToBeCompiled(true)
, mModuleMember(false)
{
}

TScript::TScript(const QString& name, Host * pHost )
: Tree<TScript>(nullptr)
, exportItem(true)
, mModuleMasterFolder(false)
, mName(name)
, mpHost(pHost)
, mNeedsToBeCompiled(true)
, mModuleMember(false)
{
}

TScript::~TScript()
{
    if (!mpHost) {
        return;
    }
    for (int i = 0; i < mEventHandlerList.size(); i++) {
        mpHost->unregisterEventHandler(mEventHandlerList[i], this);
    }
    mpHost->getScriptUnit()->unregisterScript(this);
}


bool TScript::registerScript()
{
    if (!mpHost) {
        return false;
    }
    return mpHost->getScriptUnit()->registerScript(this);
}

void TScript::setEventHandlerList(QStringList handlerList)
{
    for (int i = 0; i < mEventHandlerList.size(); i++) {
        mpHost->unregisterEventHandler(mEventHandlerList[i], this);
    }
    mEventHandlerList.clear();
    for (int i = 0; i < handlerList.size(); i++) {
        if (handlerList.at(i).isEmpty()) {
            continue;
        }
        mEventHandlerList.append(handlerList[i]);
        mpHost->registerEventHandler(handlerList[i], this);
    }
}


void TScript::compileAll(bool saveLoadingError)
{
    if (mpHost->mResetProfile) {
        mNeedsToBeCompiled = true;
    }
    compile(saveLoadingError);
    for (auto script : *mpMyChildrenList) {
        script->compileAll(saveLoadingError);
    }
}

void TScript::callEventHandler(const TEvent& pEvent)
{
    // Only call this event handler if this script and all its ancestors are active:
    if (isActive() && ancestorsActive()) {
        mpHost->mLuaInterpreter.callEventHandler(mName, pEvent);
    }
}

void TScript::compile(bool saveLoadingError)
{
    if (mNeedsToBeCompiled) {
        if (!compileScript(saveLoadingError)) {
            if (mudlet::smDebugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of script:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
}

bool TScript::setScript(const QString& script)
{
    mScript = script;
    mNeedsToBeCompiled = true;
    if (!mpHost->blockScripts()) {
        mOK_code = compileScript();
    }
    return mOK_code;
}

bool TScript::compileScript(bool saveLoadingError)
{
    QString error;
    if (mpHost->mLuaInterpreter.compile(mScript, error, QString("Script: ") + getName())) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        if (mpHost->mResetProfile) {
            setEventHandlerList(getEventHandlerList());
        }
        return true;
    } else {
        mOK_code = false;
        setError(error);
        if (saveLoadingError) {
            setLoadingError(error);
        }
        return false;
    }
}

void TScript::execute()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }
    mpHost->mLuaInterpreter.call(mFuncName, mName);
}

// Gets the Lua error message for this script if one occurred during profile load
// Returns:
// - The loading error message if there was one
// - An empty optional if no loading error occurred
std::optional<QString> TScript::getLoadingError()
{
    return mLoadingError;
}

// Sets the loading error message for this script.
// Used when an error occurs loading the script during profile load.
// The loading error can later be retrieved using getLoadingError().
//
// error: The error message to set as the loading error.
void TScript::setLoadingError(const QString& error)
{
    if (!error.isEmpty()) {
        mLoadingError = error;
    }
}

// Clears the loading error message for this script.
// Used to clear a loading error once it has been handled.
// After calling this, getLoadingError() will return an empty optional.
void TScript::clearLoadingError()
{
    mLoadingError.reset();
}

QString TScript::packageName(TScript* pScript)
{
    if (!pScript) {
        return QString();
    }

    if (!pScript->mPackageName.isEmpty()) {
        return !mpHost->mModuleInfo.contains(pScript->mPackageName) ? pScript->mPackageName : QString();
    }

    if (pScript->getParent()) {
        return packageName(pScript->getParent());
    }

    return QString();
}

QString TScript::moduleName(TScript* pScript)
{
    if (!pScript) {
        return QString();
    }

    if (!pScript->mPackageName.isEmpty()) {
        return mpHost->mModuleInfo.contains(pScript->mPackageName) ? pScript->mPackageName : QString();
    }

    if (pScript->getParent()) {
        return moduleName(pScript->getParent());
    }

    return QString();
}
