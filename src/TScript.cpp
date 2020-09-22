/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mModuleMember(false)
{
}

TScript::TScript(const QString& name, Host * pHost )
: Tree<TScript>(nullptr)
, exportItem(true)
, mModuleMasterFolder(false)
, mName( name )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
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
        if (handlerList[i].size() < 1) {
            continue;
        }
        mEventHandlerList.append(handlerList[i]);
        mpHost->registerEventHandler(handlerList[i], this);
    }
}


void TScript::compileAll()
{
    compile();
    for (auto script : *mpMyChildrenList) {
        script->compileAll();
    }
}

void TScript::callEventHandler(const TEvent& pE)
{
    // Only call this event handler if this script and all its ancestors are active:
    if (isActive() && ancestorsActive()) {
        mpHost->mLuaInterpreter.callEventHandler(mName, pE);
    }
}

void TScript::compile()
{
    if (mNeedsToBeCompiled || mpHost->mResetProfile) {
        if (!compileScript()) {
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of script:" << mName << "\n" >> 0;
            }
            mOK_code = false;
        }
    }
    for (auto script : *mpMyChildrenList) {
        script->compile();
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

bool TScript::compileScript()
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
