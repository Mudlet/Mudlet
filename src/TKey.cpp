/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2020-2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "TKey.h"


#include "Host.h"
#include "TDebug.h"
#include "mudlet.h"

TKey::TKey(TKey* parent, Host* pHost)
: Tree<TKey>( parent )
, mpHost(pHost)
{
}

TKey::TKey(QString name, Host* pHost)
: Tree<TKey>( nullptr )
, mpHost(pHost)
, mName(name)
{
}

TKey::~TKey()
{
    if (!mpHost) {
        return;
    }
    mpHost->getKeyUnit()->unregisterKey(this);

    if (isTemporary()) {
        if (mScript.isEmpty()) {
            mpHost->mLuaInterpreter.delete_luafunction(this);
        } else {
            mpHost->mLuaInterpreter.delete_luafunction(mFuncName);
        }
    }
}

void TKey::setName(const QString& name)
{
    if (!isTemporary()) {
        mpHost->getKeyUnit()->mLookupTable.remove(mName, this);
    }
    mName = name;
    mpHost->getKeyUnit()->mLookupTable.insert(name, this);
}

bool TKey::match(const Qt::Key key, const Qt::KeyboardModifiers modifier, const bool isToMatchAll)
{
    bool isAMatch = false;
    if (isActive()) {
        if (!isFolder()) {
            if ((mKeyCode == key) && (mKeyModifier == modifier)) {
                execute();
                if (isToMatchAll) {
                    isAMatch = true;
                } else {
                    return true;
                }
            }
        }

        for (auto childKey : *mpMyChildrenList) {
            if (childKey->match(key, modifier, isToMatchAll)) {
                if (isToMatchAll) {
                    isAMatch = true;
                } else {
                    return true;
                }
            }
        }
    }

    return isAMatch;
}


bool TKey::registerKey()
{
    if (!mpHost) {
        qDebug() << "ERROR: TAlias::registerTrigger() pHost=0";
        return false;
    }
    return mpHost->getKeyUnit()->registerKey(this);
}


void TKey::enableKey(const QString& name)
{
    if (mName == name) {
        setIsActive(true);
    }
    for (auto key : *mpMyChildrenList) {
        key->enableKey(name);
    }
}

void TKey::disableKey(const QString& name)
{
    if (mName == name) {
        setIsActive(false);
    }
    for (auto key : *mpMyChildrenList) {
        key->disableKey(name);
    }
}

void TKey::compileAll()
{
    mNeedsToBeCompiled = true;
    if (!compileScript()) {
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of key binding:" << mName << "\n" >> mpHost;
        }
        mOK_code = false;
    }
    for (auto key : *mpMyChildrenList) {
        key->compileAll();
    }
}

void TKey::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::smDebugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of key binding:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
    for (auto key : *mpMyChildrenList) {
        key->compile();
    }
}

bool TKey::setScript(const QString& script)
{
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TKey::compileScript()
{
    mFuncName = qsl("Key%1").arg(QString::number(mID));
    const QString code = qsl("function %1() %2\nend").arg(mFuncName, mScript);
    QString error;
    if (mpHost->mLuaInterpreter.compile(code, error, qsl("Key: %1").arg(getName()))) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        return true;
    } else {
        mOK_code = false;
        setError(error);
        return false;
    }
}

void TKey::execute()
{
    if (!mCommand.isEmpty()) {
        mpHost->send(mCommand);
    }
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }

    if (mRegisteredAnonymousLuaFunction) {
        mpHost->mLuaInterpreter.call_luafunction(this);
        return;
    }

    if (mScript.isEmpty()) {
        return;
    }

    mpHost->mLuaInterpreter.call(mFuncName, mName);
}

QString TKey::packageName(TKey* pKey)
{
    if (!pKey) {
        return QString();
    }

    if (!pKey->mPackageName.isEmpty()) {
        return !mpHost->mModuleInfo.contains(pKey->mPackageName) ? pKey->mPackageName : QString();
    }

    if (pKey->getParent()) {
        return packageName(pKey->getParent());
    }

    return QString();
}

QString TKey::moduleName(TKey* pKey)
{
    if (!pKey) {
        return QString();
    }

    if (!pKey->mPackageName.isEmpty()) {
        return mpHost->mModuleInfo.contains(pKey->mPackageName) ? pKey->mPackageName : QString();
    }

    if (pKey->getParent()) {
        return moduleName(pKey->getParent());
    }

    return QString();
}
