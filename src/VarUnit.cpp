#include "VarUnit.h"
#include <QDebug>
#include <QTreeWidgetItem>

VarUnit::VarUnit()
{
}

void VarUnit::buildVarTree( QTreeWidgetItem * p, TVar * var ){
    QStringList sl;
    sl << var->getName();
    QTreeWidgetItem * pItem = new QTreeWidgetItem(p, sl);
    if (var->getValueType() == 5){
        QListIterator<TVar *> it(var->getChildren());
        while(it.hasNext()){
            TVar * child = it.next();
            if (child->getValueType() == 5)
                buildVarTree( pItem, child );
        }
    }
}

QStringList * VarUnit::varName(TVar * var){
    QStringList names;
    names << "_G";
    if (var == base){
        return &names;
    }
    names << var->getName();
    TVar * p = var->getParent();
    while (p != base){
        names.insert(1,p->getName());
        if (p == base)
            break;
        p = p->getParent();
    }
    return &names;
}

void VarUnit::addVariable(TVar * var){
    varList.insert(varName(var));
    vars.insert(var);
}

bool VarUnit::varExists(TVar * var){
    QStringList * names = varName(var);
    if (var->getValueType() == 5){
        if (names->count(var->getName())>1)
            return true;
    }
    return varList.contains(names);
}

TVar * VarUnit::getBase(){
    return base;
}

void VarUnit::setBase(TVar * t){
    base = t;
}

void VarUnit::clear(){
    QSetIterator<TVar *> i(vars);
    while (i.hasNext())
        delete i.next();
    vars.clear();
    varList.clear();
}
