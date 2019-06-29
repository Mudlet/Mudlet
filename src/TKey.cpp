/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2019 by Stephen Lyons - slysven@virginmedia.com    *
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


using namespace std;

TKey::TKey(TKey* parent, Host* pHost)
    : Tree<TKey>( parent )
    , exportItem(true)
    , mModuleMasterFolder(false)
    , mpHost( pHost )
    , mNeedsToBeCompiled( true )
    , mModuleMember(false)
    , mKeyCode()
    , mPresentModifiers()
    , mAbsentModifiers()
{
}

TKey::TKey(QString name, Host* pHost)
    : Tree<TKey>( nullptr )
    , exportItem( true )
    , mModuleMasterFolder( false )
    , mName( name )
    , mpHost( pHost )
    , mNeedsToBeCompiled( true )
    , mModuleMember(false)
    , mKeyCode()
    , mPresentModifiers()
    , mAbsentModifiers()
{
}

TKey::~TKey()
{
    if (!mpHost) {
        return;
    }
    mpHost->getKeyUnit()->unregisterKey(this);
}

void TKey::setName(const QString& name)
{
    if (!isTemporary()) {
        mpHost->getKeyUnit()->mLookupTable.remove(mName, this);
    }
    mName = name;
    mpHost->getKeyUnit()->mLookupTable.insertMulti(name, this);
}

bool TKey::match(const int key, const Qt::KeyboardModifiers modifiers, const bool isToMatchAll)
{
    bool isAMatch = false;
    if (isActive()) {
        if (!isFolder()) {
            if (  (mKeyCode == key) // The key matches
                    && ((modifiers & mPresentModifiers) == mPresentModifiers) // All of the required present modifiers are present and thus this matches
                    && (!(modifiers & mAbsentModifiers))) { // None of the required to be absent modifiers are present and thus this matches

                execute();
                if (isToMatchAll) {
                    isAMatch = true;
                } else {
                    return true;
                }
            }
        }

        for (auto childKey : *mpMyChildrenList) {
            if (childKey->match(key, modifiers, isToMatchAll)) {
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
        if (mudlet::debugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of key binding:" << mName << "\n" >> 0;
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
            if (mudlet::debugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of key binding:" << mName << "\n" >> 0;
            }
            mOK_code = false;
        }
    }
    for (auto key : *mpMyChildrenList) {
        key->compile();
    }
}

bool TKey::setScript(QString& script)
{
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TKey::compileScript()
{
    mFuncName = QStringLiteral("Key%1").arg(QString::number(mID));
    QString code = QStringLiteral("function %1()\n%2\nend\n").arg(mFuncName, mScript);
    QString error;
    if (mpHost->mLuaInterpreter.compile(code, error, QStringLiteral("Key: %1").arg(getName()))) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
    } else {
        mOK_code = false;
        setError(error);
    }
    return mOK_code;
}

void TKey::execute()
{
    if (mCommand.size() > 0) {
        mpHost->send(mCommand);
    }
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }
    mpHost->mLuaInterpreter.call(mFuncName, mName);
}
