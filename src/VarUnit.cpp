#include "VarUnit.h"
#include <QDebug>
#include <QTreeWidgetItem>

VarUnit::VarUnit()
{
    wVars.setInsertInOrder(true);
}

bool VarUnit::isHidden( TVar * var ){
    if ( var->getName() == "_G" )//we never hide global
        return false;
//    qDebug()<<"checking"<<var->getName()<<shortVarName(var).join(".");
    return hidden.contains(shortVarName(var).join("."));
}

void VarUnit::buildVarTree( QTreeWidgetItem * p, TVar * var, bool showHidden ){
    QList< QTreeWidgetItem * > cList;
    QListIterator<TVar *> it(var->getChildren());
    while(it.hasNext()){
        TVar * child = it.next();
//        qDebug()<<child->getName()<<isHidden(child);
        if ( showHidden || !isHidden( child ) ){
            QStringList s1;
            s1 << child->getName();
            QTreeWidgetItem * pItem = new QTreeWidgetItem(s1);
            pItem->setText( 0, child->getName() );
            pItem->setFlags(Qt::ItemIsTristate|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled);
            if ( isSaved( child ) ){
                pItem->setCheckState(0, Qt::Checked);
            }
            else
                pItem->setCheckState(0, Qt::Unchecked);
            pItem->setToolTip(0, "Checked variables will be saved and loaded with your profile.");
            wVars.insert( pItem, child );
            cList.append( pItem );
            if ( child->getValueType() == 5 )
                buildVarTree( pItem, child, showHidden );
        }
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
    if (var == base || ! var ){
        return &names;
    }
    names << var->getName();
    TVar * p = var->getParent();
    while (p && p != base){
        names.insert(1,p->getName());
        if (p == base)
            break;
        p = p->getParent();
    }
    return &names;
}

QStringList VarUnit::shortVarName(TVar * var){
    QStringList names;
    if (var->getName() == "_G"){
        names << "";
        return names;
    }
    names << var->getName();
    TVar * p = var->getParent();
    while (p && p->getName() != "_G"){
        names.insert(0,p->getName());
        p = p->getParent();
    }
    return names;
}

void VarUnit::addVariable(TVar * var){
    QStringList * n = varName(var);
    varList.insert(n);
    if ( var->hidden ){
        hidden.insert(shortVarName(var).join("."));
    }
}

void VarUnit::addHidden( TVar * var ){
    var->hidden = true;
    hidden.insert(shortVarName(var).join("."));
}

void VarUnit::removeHidden( TVar * var ){
    hidden.remove(shortVarName(var).join("."));
    var->hidden = false;
}

void VarUnit::addSavedVar(TVar * var){
    QString n = shortVarName(var).join(".");
//    qDebug()<<n;
    savedVars.insert(n);
//    qDebug()<<"saved"<<savedVars;
}

void VarUnit::removeSavedVar(TVar * var){
    QString n = shortVarName(var).join(".");
//    savedVars.remove(n);
//    qDebug()<<"removed"<<savedVars;
}

bool VarUnit::isSaved( TVar * var ){
    QString n = shortVarName(var).join(".");
    return savedVars.contains(n);
}

void VarUnit::removeVariable(TVar * var){
//    TVar * parent = var->getParent();
//    parent->removeChild(var);
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
    delete base;
    tVars.clear();
    wVars.clear();
    varList.clear();
}
