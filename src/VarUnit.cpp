/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021-2022 by Stephen Lyons - slysven@virginmedia..com   *
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


#include "VarUnit.h"

#include "TVar.h"

#include <QTreeWidgetItem>
#include <QDebug>


VarUnit::VarUnit()
: base()
{
}

bool VarUnit::isHidden(TVar* var)
{
    if (var->getName() == qsl("_G")) { // we never hide global
        return false;
    }
    if (hidden.contains(shortVarName(var).join(qsl(".")))) {
        return true;
    }
    return hiddenByUser.contains(shortVarName(var).join(qsl(".")));
}


bool VarUnit::isHidden(const QString& fullname)
{
    if (fullname == QLatin1String("_G")) { // we never hide global
        return false;
    }
    if (hidden.contains(fullname)) {
        return true;
    }
    return hiddenByUser.contains(fullname);
}

void VarUnit::addPointer(const void* pointer)
{
    mPointers.insert(pointer);
}

bool VarUnit::shouldSave(QTreeWidgetItem* pWidgetItem)
{
    auto var = getWVar(pWidgetItem);

    return !(!var || var->getValueType() == 6 || var->isReference());
}

bool VarUnit::shouldSave(TVar* var)
{
    return !(var->getValueType() == 6 || var->isReference());
}

void VarUnit::buildVarTree(QTreeWidgetItem* p, TVar* var, bool showHidden)
{
    QList<QTreeWidgetItem*> cList;
    QListIterator<TVar*> it(var->getChildren(true));
    while (it.hasNext()) {
        TVar* child = it.next();
        if (showHidden || !isHidden(child)) {
            QStringList s1;
            s1 << child->getName();
            auto pItem = new QTreeWidgetItem(s1);
            pItem->setText(0, child->getName());
            pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
            pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
            pItem->setCheckState(0, Qt::Unchecked);
            if (isSaved(child)) {
                pItem->setCheckState(0, Qt::Checked);
            }
            if (!shouldSave(child)) { // 6 is lua_tfunction, parent must be saveable as well if not global
                pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
                pItem->setForeground(0, QBrush(QColor("grey")));
                pItem->setToolTip(0, QString());
            }
            pItem->setData(0, Qt::UserRole, child->getValueType());
            QIcon icon;
            switch (child->getValueType()) {
            case 5:
                icon.addPixmap(QPixmap(qsl(":/icons/table.png")), QIcon::Normal, QIcon::Off);
                break;
            case 6:
                icon.addPixmap(QPixmap(qsl(":/icons/function.png")), QIcon::Normal, QIcon::Off);
                break;
            default:
                icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
                break;
            }
            pItem->setIcon(0, icon);
            wVars.insert(pItem, child);
            cList.append(pItem);
            if (child->getValueType() == 5) {
                buildVarTree(pItem, child, showHidden);
            }
        }
    }
    p->addChildren(cList);
}

void VarUnit::addTreeItem(QTreeWidgetItem* p, TVar* var)
{
    wVars.insert(p, var);
}

void VarUnit::addTempVar(QTreeWidgetItem* p, TVar* var)
{
    tVars.insert(p, var);
}

void VarUnit::removeTempVar(QTreeWidgetItem* p)
{
    tVars.remove(p);
}

TVar* VarUnit::getTVar(QTreeWidgetItem* p)
{
    if (tVars.contains(p)) {
        return tVars[p];
    }
    return nullptr;
}

TVar* VarUnit::getWVar(QTreeWidgetItem* p)
{
    if (wVars.contains(p)) {
        return wVars[p];
    }
    return nullptr;
}

QStringList VarUnit::varName(TVar* var)
{
    QStringList names;
    names << "_G";
    if (var == base || !var) {
        return names;
    }
    names << var->getName();
    TVar* p = var->getParent();
    while (p && p != base) {
        names.insert(1, p->getName());
        if (p == base) {
            break;
        }
        p = p->getParent();
    }
    return names;
}

QStringList VarUnit::shortVarName(TVar* var)
{
    QStringList names;
    if (!var || var->getName() == qsl("_G")) {
        names << "";
        return names;
    }
    names << var->getName();
    TVar* pParent = var->getParent();
    while (pParent && pParent->getName() != qsl("_G")) {
        names.insert(0, pParent->getName());
        pParent = pParent->getParent();
    }
    return names;
}

void VarUnit::addVariable(TVar* var)
{
    const QString fullName = varName(var).join(qsl("."));
    // pointers.insert(var->pointer);
    variableSet.insert(fullName);
    if (var->hidden) {
        hidden.insert(shortVarName(var).join(qsl(".")));
    }
}

void VarUnit::addHidden(TVar* var, int user)
{
    var->hidden = true;
    if (user) {
        hiddenByUser.insert(shortVarName(var).join(qsl(".")));
    } else {
        hidden.insert(shortVarName(var).join(qsl(".")));
    }
}

void VarUnit::addHidden(const QString& var)
{
    hiddenByUser.insert(var);
}

void VarUnit::removeHidden(TVar* var)
{
    const QString fullName = shortVarName(var).join(qsl("."));
    hidden.remove(fullName);
    hiddenByUser.remove(fullName);
    var->hidden = false;
}

void VarUnit::removeHidden(const QString& name)
{
    hidden.remove(name);
    hiddenByUser.remove(name);
    // does not remove the reference from TVar, similar to addHidden()
}

void VarUnit::addSavedVar(TVar* var)
{
    const QString fullName = shortVarName(var).join(qsl("."));
    var->saved = true;
    savedVars.insert(fullName);
}

void VarUnit::removeSavedVar(TVar* var)
{
    const QString fullName = shortVarName(var).join(qsl("."));
    savedVars.remove(fullName);
    var->saved = false;
}

bool VarUnit::isSaved(TVar* var)
{
    const QString fullName = shortVarName(var).join(qsl("."));
    return (savedVars.contains(fullName) || var->saved);
}

void VarUnit::removeVariable(TVar* var)
{
    variableSet.remove(varName(var).join(qsl(".")));
}

bool VarUnit::varExists(TVar* var)
{
    return ((var->pKey && mPointers.contains(var->pKey)) || (var->pValue && mPointers.contains(var->pValue)));
}

TVar* VarUnit::getBase()
{
    return base;
}

void VarUnit::setBase(TVar* pVariable)
{
    base = pVariable;
}

void VarUnit::clear()
{
    // delete base;
    tVars.clear();
    wVars.clear();
    variableSet.clear();
    mPointers.clear();
}
