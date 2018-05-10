/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018 by Stephen Lyons - slysven@virginmedia.com         *
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
, mKeyModifier()
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
, mKeyModifier()
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

bool TKey::match(int key, int modifier, const bool isToMatchAll)
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
    mFuncName = QString("Key") + QString::number(mID);
    QString code = QString("function ") + mFuncName + QString("()\n") + mScript + QString("\nend\n");
    QString error;
    if (mpHost->mLuaInterpreter.compile(code, error, QString("Key: ") + getName())) {
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
