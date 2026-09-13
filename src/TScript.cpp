/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021, 2026 by Stephen Lyons - slysven@virginmedia.com   *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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
#include "ScriptUnit.h"
#include "TDebug.h"
#include "TLuaInterpreter.h"

#include <QColor>
#include <QMap>
#include <QScopeGuard>

#include <list>
#include <utility>

TScript::TScript(TScript* parent, Host* pHost)
: Tree<TScript>(parent)
, mpHost(pHost)
{
}

TScript::TScript(const QString& name, Host* pHost)
: Tree<TScript>(nullptr)
, mName(name)
, mpHost(pHost)
{
}

TScript::~TScript()
{
    if (!mpHost) {
        return;
    }
    for (const auto& handler : std::as_const(mEventHandlerList)) {
        mpHost->unregisterEventHandler(handler, this);
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
    for (const QString& handler : std::as_const(mEventHandlerList)) {
        mpHost->unregisterEventHandler(handler, this);
    }
    mEventHandlerList.clear();
    for (const QString& handler : handlerList) {
        if (handler.isEmpty()) {
            continue;
        }
        mEventHandlerList.append(handler);
        mpHost->registerEventHandler(handler, this);
    }
}


void TScript::compileAll(bool saveLoadingError)
{
    if (mpHost->mResetProfile) {
        mNeedsToBeCompiled = true;
    }
    compile(saveLoadingError);
    for (auto* scriptNode : *mpMyChildrenList) {
        auto* script = static_cast<TScript*>(scriptNode);
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
            if (TDebug::wants(TDebug::Category::Error)) {
                TDebug(Qt::white, Qt::red, TDebug::Category::Error, mName) << "ERROR: Lua compile error. compiling script of script:" << mName << "\n" >> mpHost;
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
    // Whilst this frame is on the stack ScriptUnit::uninstall() must defer deleting
    // this profile's scripts: the top-level Lua body run below (the lua_pcall inside
    // TLuaInterpreter::compile()) can uninstall its own package - a common package
    // auto-updater pattern - and freeing this script mid-compile, or writing to it
    // after compile() returns (see mNeedsToBeCompiled/mOK_code below and in
    // setScript()), is a use-after-free. See ScriptUnit::mProcessingDepth.
    ScriptUnit* pUnit = mpHost->getScriptUnit();
    pUnit->beginProcessing();
    // NB: deliberately decrement-only - do NOT add a doCleanup() call here. setScript()
    // writes mOK_code AFTER this returns and ScriptUnit::compileAll()'s loop is still
    // iterating the root list, so deleting `this` now would be a use-after-free. The
    // deferred deletes are flushed at a safe point once the pointer is no longer in
    // use: after ScriptUnit::compileAll()'s loop, at the end of the editor's
    // saveScript(), in Host::raiseEvent()'s scope guard, and by the catch-all
    // doCleanup() in Host::incomingStreamProcessor()/slot_purgeTemps() and the queued
    // save in Host::uninstallPackage().
    const auto processingGuard = qScopeGuard([pUnit] {
        pUnit->endProcessing();
    });

    QString error;
    if (mpHost->mLuaInterpreter.compile(mScript, error, QString("Script: ") + getName())) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        if (mpHost->mResetProfile) {
            setEventHandlerList(getEventHandlerList());
        }
        return true;
    }
    mOK_code = false;
    setError(error);
    if (saveLoadingError) {
        setLoadingError(error);
    }
    return false;
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
        return !mpHost->mInstalledModules.contains(pScript->mPackageName) ? pScript->mPackageName : QString();
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
        return mpHost->mInstalledModules.contains(pScript->mPackageName) ? pScript->mPackageName : QString();
    }

    if (pScript->getParent()) {
        return moduleName(pScript->getParent());
    }

    return QString();
}

bool TScript::checkIfNew()
{
    return mIsNew;
}

void TScript::unmarkAsNew()
{
    mIsNew = false;
}
