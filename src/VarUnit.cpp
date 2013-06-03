#include "VarUnit.h"
#include <QDebug>
#include <QTreeWidgetItem>

VarUnit::VarUnit()
{
}

void VarUnit::buildVarTree( QTreeWidgetItem * p, TVar * var ){
    //we start here with the global namespace
    QList< QTreeWidgetItem * > cList;
    QListIterator<TVar *> it(var->getChildren());
    while(it.hasNext()){
        TVar * child = it.next();
        QStringList s1;
        s1 << child->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(s1);
        wVars.insert( pItem, child );
        cList.append( pItem );
        if ( child->getValueType() == 5 )
            buildVarTree( pItem, child );
    }
    p->addChildren( cList );
}

TVar * VarUnit::getWVar( QTreeWidgetItem * p ){
    return wVars[p];
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
}

void VarUnit::removeVariable(TVar * var){
    varList.remove(varName(var));
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
    wVars.clear();
    varList.clear();
}
