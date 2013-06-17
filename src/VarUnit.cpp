#include "VarUnit.h"
#include <QDebug>
#include <QTreeWidgetItem>
#include <QStringList>

VarUnit::VarUnit()
{
    qDebug()<<"making new VarUnit";
    wVars.setInsertInOrder(true);
}

bool VarUnit::isHidden( TVar * var ){
    if ( var->getName() == "_G" )//we never hide global
        return false;
//    qDebug()<<"checking"<<var->getName()<<shortVarName(var).join(".");
    if (hidden.contains(shortVarName(var).join(".")))
        return true;
    return hiddenByUser.contains(shortVarName(var).join("."));
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
            pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled);
            if (child->getValueType() != 6)//6 is lua_tfunction
                pItem->setFlags(pItem->flags()|Qt::ItemIsTristate|Qt::ItemIsUserCheckable);
            if ( isSaved( child ) ){
                pItem->setCheckState(0, Qt::Checked);
            }
            else if (child->getValueType() != 6)
                pItem->setCheckState(0, Qt::Unchecked);
            pItem->setData( 0, Qt::UserRole, child->getValueType() );
            QIcon icon;
            switch (child->getValueType()){
                case 5:
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/table.png")), QIcon::Normal, QIcon::Off);
                    break;
                case 6:
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/function.png")), QIcon::Normal, QIcon::Off);
                    break;
                default:
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
                    break;
            }
            pItem->setIcon( 0, icon );
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
    if (tVars.contains(p))
        return tVars[p];
    return 0;
}

TVar * VarUnit::getWVar( QTreeWidgetItem * p ){
    if (wVars.contains(p))
        return wVars[p];
    return 0;
}

QStringList VarUnit::varName(TVar * var){
    QStringList names;
    names << "_G";
    if (var == base || ! var ){
        return names;
    }
    names << var->getName();
    TVar * p = var->getParent();
    while (p && p != base){
        names.insert(1,p->getName());
        if (p == base)
            break;
        p = p->getParent();
    }
    return names;
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
    QString n = varName(var).join(".");
    varList.insert(n);
    if ( var->hidden ){
        hidden.insert(shortVarName(var).join("."));
    }
}

void VarUnit::addHidden( TVar * var, int user ){
    var->hidden = true;
    if ( user )
        hiddenByUser.insert(shortVarName(var).join("."));
    else
        hidden.insert(shortVarName(var).join("."));
}

void VarUnit::addHidden( QString var ){
    hiddenByUser.insert(var);
}

void VarUnit::removeHidden( TVar * var ){
    QString n = shortVarName(var).join(".");
    hidden.remove(n);
    hiddenByUser.remove(n);
    var->hidden = false;
}

void VarUnit::addSavedVar(TVar * var){
    QString n = shortVarName(var).join(".");
//    qDebug()<<n;
    savedVars.insert(n);
    qDebug()<<"added"<<n;
}

void VarUnit::removeSavedVar(TVar * var){
    QString n = shortVarName(var).join(".");
    savedVars.remove(n);
    //qDebug()<<"removed"<<n;
}

bool VarUnit::isSaved( TVar * var ){
    QString n = shortVarName(var).join(".");
//    qDebug()<<"checking isSaved"<<n;
    return savedVars.contains(n);
}

void VarUnit::removeVariable(TVar * var){
//    TVar * parent = var->getParent();
//    parent->removeChild(var);
    varList.remove(varName(var).join("."));
}

bool VarUnit::varExists(TVar * var){
    QStringList names = varName(var);
    if (var->getValueType() == 5){
        QString name = var->getName();
        if (names.count(name)>1)
            return true;
//        for(int i=0;i<names.size();i++){
//            if (name == names.at(i))
//                return true;
//        }
    }
    return varList.contains(names.join("."));
}

TVar * VarUnit::getBase(){
    return base;
}

void VarUnit::setBase(TVar * t){
    base = t;
}

void VarUnit::clear(){
    //delete base;
    tVars.clear();
    wVars.clear();
    varList.clear();
}
