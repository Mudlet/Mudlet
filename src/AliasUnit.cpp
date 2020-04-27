/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "AliasUnit.h"

#include "Host.h"
#include "TAlias.h"

void AliasUnit::_uninstall(TAlias* pChild, const QString& packageName)
{
    std::list<TAlias*>* childrenList = pChild->mpMyChildrenList;
    for (auto alias : *childrenList) {
        _uninstall(alias, packageName);
        uninstallList.append(alias);
    }
}


void AliasUnit::uninstall(const QString &packageName)
{
    for (auto rootAlias : mAliasRootNodeList) {
        if (rootAlias->mPackageName == packageName) {
            _uninstall(rootAlias, packageName);
            uninstallList.append(rootAlias);
        }
    }
    for (auto& alias : uninstallList) {
        unregisterAlias(alias);
    }
    uninstallList.clear();
}

void AliasUnit::compileAll()
{
    for (auto alias : mAliasRootNodeList) {
        if (alias->isActive()) {
            alias->compileAll();
        }
    }
}

void AliasUnit::initStats()
{
    statsAliasTotal = 0;
    statsTempAliases = 0;
    statsActiveAliases = 0;
    statsActiveAliasesMax = 0;
    statsActiveAliasesMin = 0;
    statsActiveAliasesAverage = 0;
    statsTempAliasesCreated = 0;
    statsTempAliasesKilled = 0;
    statsAverageLineProcessingTime = 0;
    statsMaxLineProcessingTime = 0;
    statsMinLineProcessingTime = 0;
    statsRegexAliases = 0;
}

void AliasUnit::addAliasRootNode(TAlias* pT, int parentPosition, int childPosition, bool moveAlias)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }
    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mAliasRootNodeList.size()))) {
        mAliasRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mAliasRootNodeList.insert(it, pT);
                break;
            }
            cnt++;
        }
    }

    if (!moveAlias) {
        mAliasMap.insert(pT->getID(), pT);
    }
}

void AliasUnit::reParentAlias(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    TAlias* pOldParent = getAliasPrivate(oldParentID);
    TAlias* pNewParent = getAliasPrivate(newParentID);
    TAlias* pChild = getAliasPrivate(childID);
    if (!pChild) {
        return;
    }
    if (pOldParent) {
        pOldParent->popChild(pChild);
    } else {
        mAliasRootNodeList.remove(pChild);
    }
    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        pChild->setParent(pNewParent);
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    } else {
        pChild->Tree<TAlias>::setParent(nullptr);
        addAliasRootNode(pChild, parentPosition, childPosition, true);
    }
}

void AliasUnit::removeAliasRootNode(TAlias* pT)
{
    if (!pT) {
        return;
    }
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->mName, pT);
    } else {
        mLookupTable.remove(pT->getName());
    }
    mAliasMap.remove(pT->getID());
    mAliasRootNodeList.remove(pT);
}

void AliasUnit::removeAllTempAliases()
{
    for (auto alias : mAliasRootNodeList) {
        if (alias->isTemporary()) {
            alias->setIsActive(false);
            markCleanup(alias);
        }
    }
}

TAlias* AliasUnit::getAlias(int id)
{
    QMutexLocker locker(&mAliasUnitLock);
    if (mAliasMap.find(id) != mAliasMap.end()) {
        return mAliasMap.value(id);
    } else {
        return nullptr;
    }
}

TAlias* AliasUnit::getAliasPrivate(int id)
{
    if (mAliasMap.find(id) != mAliasMap.end()) {
        return mAliasMap.value(id);
    } else {
        return nullptr;
    }
}

bool AliasUnit::registerAlias(TAlias* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addAlias(pT);
        return true;
    } else {
        addAliasRootNode(pT);
        return true;
    }
}

void AliasUnit::unregisterAlias(TAlias* pT)
{
    if (!pT) {
        return;
    }
    if (pT->getParent()) {
        removeAlias(pT);
        return;
    } else {
        removeAliasRootNode(pT);
        return;
    }
}


void AliasUnit::addAlias(TAlias* pT)
{
    if (!pT) {
        return;
    }

    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    mAliasMap.insert(pT->getID(), pT);
}

void AliasUnit::removeAlias(TAlias* pT)
{
    if (!pT) {
        return;
    }
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->mName, pT);
    } else {
        mLookupTable.remove(pT->getName());
    }

    mAliasMap.remove(pT->getID());
}


int AliasUnit::getNewID()
{
    return ++mMaxID;
}

bool AliasUnit::processDataStream(const QString& data)
{
    TLuaInterpreter* Lua = mpHost->getLuaInterpreter();
    Lua->set_lua_string(QStringLiteral("command"), data);
    bool state = false;
    for (auto alias : mAliasRootNodeList) {
        // = data.replace( "\n", "" );
        if (alias->match(data)) {
            state = true;
        }
    }
    // the idea to get "command" after alias processing is finished and send its value
    // was too difficult for users because if multiple alias change the value of command it becomes too difficult to handle for many users
    // it's easier if we simply intercepts the command and hand responsibility for
    // sending a command to the user scripts.
    //data = Lua->get_lua_string( lua_command_string );
    return state;
}


void AliasUnit::stopAllTriggers()
{
    for (auto alias : mAliasRootNodeList) {
        alias->disableFamily();
    }
}

void AliasUnit::reenableAllTriggers()
{
    for (auto alias : mAliasRootNodeList) {
        alias->enableFamily();
    }
}

TAlias* AliasUnit::findFirstAlias(const QString& name)
{
    QMap<QString, TAlias*>::const_iterator it = mLookupTable.constFind(name);
    if (it != mLookupTable.cend() && it.key() == name) {
        return it.value();
    }
    return nullptr;
}

bool AliasUnit::enableAlias(const QString& name)
{
    bool found = false;
    QMap<QString, TAlias*>::const_iterator it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TAlias* pT = it.value();
        pT->setIsActive(true);
        ++it;
        found = true;
    }
    return found;
}

bool AliasUnit::disableAlias(const QString& name)
{
    bool found = false;
    QMap<QString, TAlias*>::const_iterator it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TAlias* pT = it.value();
        pT->setIsActive(false);
        ++it;
        found = true;
    }
    return found;
}


bool AliasUnit::killAlias(const QString& name)
{
    for (auto alias : mAliasRootNodeList) {
        if (alias->getName() == name) {
            // only temporary Aliases can be killed
            if (!alias->isTemporary()) {
                return false;
            } else {
                alias->setIsActive(false);
                markCleanup(alias);
                return true;
            }
        }
    }
    return false;
}

void AliasUnit::_assembleReport(TAlias* pChild)
{
    std::list<TAlias*>* childrenList = pChild->mpMyChildrenList;
    for (auto alias : *childrenList) {
        _assembleReport(alias);
        if (alias->isActive()) {
            statsActiveAliases++;
        }
        if (alias->isTemporary()) {
            statsTempAliases++;
        }
        statsAliasTotal++;
    }
}

QString AliasUnit::assembleReport()
{
    statsActiveAliases = 0;
    statsAliasTotal = 0;
    statsTempAliases = 0;
    for (auto alias : mAliasRootNodeList) {
        if (alias->isActive()) {
            statsActiveAliases++;
        }
        if (alias->isTemporary()) {
            statsTempAliases++;
        }
        statsAliasTotal++;
        std::list<TAlias*>* childrenList = alias->mpMyChildrenList;
        for (auto childAlias : *childrenList) {
            _assembleReport(childAlias);
            if (childAlias->isActive()) {
                statsActiveAliases++;
            }
            if (childAlias->isTemporary()) {
                statsTempAliases++;
            }
            statsAliasTotal++;
        }
    }
    QStringList msg;
    msg << QStringLiteral("Aliases current total: ") << QString::number(statsAliasTotal) << QStringLiteral("\n")
        << QStringLiteral("tempAliases current total: ") << QString::number(statsTempAliases) << QStringLiteral("\n")
        << QStringLiteral("active Aliases: ") << QString::number(statsActiveAliases) << QStringLiteral("\n");
        /*<< "active Aliases max this session: " << QString::number(statsActiveAliasesMax) << "\n"
        << "active Aliases min this session: " << QString::number(statsActiveAliasesMin) << "\n"
        << "active Aliases average this session: " << QString::number(statsActiveAliasesAverage) << "\n"*/
        //<< "tempAliases created this session: " << QString::number(statsTempAliasesCreated) << "\n"
        //<< "tempAliases killed this session: " << QString::number(statsTempAliasesKilled) << "\n"
        //<< "current total regex Aliases: " << QString::number(statsRegexAliases) << "\n"
        //<< "average line processing time: " << QString::number(statsAverageLineProcessingTime) << "\n"
        //<< "max line processing time: " << QString::number(statsMaxLineProcessingTime) << "\n"
        //<< "min line processing time: " << QString::number(statsMinLineProcessingTime) << "\n";
    return msg.join(QString());
}

void AliasUnit::doCleanup()
{
    for (auto alias : mCleanupList) {
        delete alias;
    }
    mCleanupList.clear();
}

void AliasUnit::markCleanup(TAlias* pT)
{
    for (auto alias : mCleanupList) {
        if (alias == pT) {
            return;
        }
    }
    mCleanupList.push_back(pT);
}
