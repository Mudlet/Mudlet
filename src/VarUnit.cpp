#include "VarUnit.h"
#include <QDebug>
#include <QTreeWidgetItem>

VarUnit::VarUnit()
{
    wVars.setInsertInOrder(true);
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
        pItem->setText( 0, child->getName() );
        pItem->setFlags(Qt::ItemIsTristate|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled);
        pItem->setCheckState(0, Qt::Unchecked);
        pItem->setToolTip(0, "Checked variables will be saved and loaded with your profile.");
        wVars.insert( pItem, child );
        cList.append( pItem );
        if ( child->getValueType() == 5 )
            buildVarTree( pItem, child );
    }
    p->addChildren( cList );
}

void VarUnit::addTreeItem( QTreeWidgetItem * p, TVar * var ){
    wVars.insert( p, var );
}

void VarUnit::addTempVar( QTreeWidgetItem * p, TVar * var ){
    tVars.insert( p, var );
}

void VarUnit::removeTempVar( QTreeWidgetItem * p ){
    tVars.remove( p );
}

TVar * VarUnit::getTVar( QTreeWidgetItem * p ){
    qDebug()<<tVars;
    if (tVars.contains(p))
        return tVars[p];
    return 0;
}

TVar * VarUnit::getWVar( QTreeWidgetItem * p ){
    if (wVars.contains(p))
        return wVars[p];
    return 0;
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
    TVar * parent = var->getParent();
    parent->removeChild(var);
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
