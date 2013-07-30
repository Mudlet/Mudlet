#include "VarUnit.h"
#include <QDebug>
#include <QTreeWidgetItem>
#include "TTreeWidget.h"
#include <QStringList>

VarUnit::VarUnit()
{
    qDebug()<<"making new VarUnit";
//    wVars.setInsertInOrder(true);
}

bool VarUnit::isHidden( TVar * var ){
    if ( var->getName() == "_G" )//we never hide global
        return false;
//    qDebug()<<"checking"<<var->getName()<<shortVarName(var).join(".");
    if (hidden.contains(shortVarName(var).join(".")))
        return true;
    return hiddenByUser.contains(shortVarName(var).join("."));
}

void VarUnit::addPointer( const void *p ){
    pointers.insert(p);
}

bool VarUnit::shouldSave( QTreeWidgetItem * p ){
    TVar * var = getWVar(p);
    if ( var->getValueType() == 6 || var->isReference() )
        return false;
    return true;
}

bool VarUnit::shouldSave( TVar * var ){
    if ( var->getValueType() == 6 || var->isReference() )
        return false;
    return true;
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
            pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsTristate|Qt::ItemIsUserCheckable);
            pItem->setToolTip(0, "Checked variables will be saved and loaded with your profile.");
            pItem->setCheckState(0, Qt::Unchecked);
            if ( isSaved( child ) )
                pItem->setCheckState(0, Qt::Checked);
            if ( ! shouldSave( child ) ){//6 is lua_tfunction, parent must be saveable as well if not global
//                pItem->setFlags(pItem->flags() & ~(Qt::ItemIsUserCheckable));
                pItem->setDisabled( true );
                pItem->setToolTip(0, "");
//                pItem->setData(0, Qt::CheckStateRole, QVariant());
//                qDebug()<<child->getName();
            }
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
            wVars.insert( pItem, child );
            cList.append( pItem );
            if ( child->getValueType() == 5 )
                buildVarTree( (QTreeWidgetItem *)pItem, child, showHidden );
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
    if (!var || var->getName() == "_G"){
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
//    pointers.insert(var->pointer);
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
    var->saved = true;
//    qDebug()<<n;
    savedVars.insert(n);
    qDebug()<<"added"<<n;
}

void VarUnit::removeSavedVar(TVar * var){
    QString n = shortVarName(var).join(".");
    savedVars.remove(n);
    var->saved = false;
    //qDebug()<<"removed"<<n;
}

bool VarUnit::isSaved( TVar * var ){
    QString n = shortVarName(var).join(".");
//    qDebug()<<"checking isSaved"<<n;
    return (savedVars.contains(n) || var->saved);
}

void VarUnit::removeVariable(TVar * var){
//    TVar * parent = var->getParent();
//    parent->removeChild(var);
//    if (var->getValueType() == 5)
//        pointers.remove(var->pointer);
    varList.remove(varName(var).join("."));
}

bool VarUnit::varExists(TVar * var){
    return ( (var->kpointer && pointers.contains(var->kpointer)) ||
             (var->vpointer && pointers.contains(var->vpointer)));
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
    pointers.clear();
}
