#include "TVar.h"

TVar::TVar()
{
}

TVar::TVar(TVar * p)
{
    parent = p;
}

TVar::TVar(TVar * p, QString kName, int kt, QString val, int vt){
    parent = p;
    name = kName;
    kType = kt;
    value = val;
    vType = vt;
}

void TVar::addChild(TVar * c){
    children.append(c);
}

QString TVar::getName(){
    return name;
}

QList<TVar *> TVar::getChildren(){
    return children;
}

void TVar::setParent(TVar * t){
    parent = t;
}

void TVar::removeChild(TVar * t){
    children.removeAll(t);
}

int TVar::getKeyType(){
    return kType;
}

int TVar::getValueType(){
    return vType;
}

bool TVar::setValue(QString val){
    value = val;
    return 1;
}

bool TVar::setValue(QString val, int t){
    value = val;
    vType = t;
    return 1;
}

bool TVar::setValueType(int t){
    vType = t;
    return 1;
}

bool TVar::setName(QString n, int kt){
    name = n;
    kType = kt;
}

bool TVar::setName(QString n){
    name = n;
}

TVar * TVar::getParent(){
    return parent;
}
