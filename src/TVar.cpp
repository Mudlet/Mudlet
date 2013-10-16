/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   <email Chris>                                                         *
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

#include "TVar.h"
#include <QDebug>

TVar::TVar()
{
    hidden = false;
    name = "";
    reference = false;
    vpointer = 0;
    kpointer = 0;
    saved = false;
}

TVar::TVar(TVar * p)
{
    parent = p;
    hidden = false;
    name = "";
    reference = false;
    vpointer = 0;
    kpointer = 0;
    saved = false;
}

TVar::TVar(TVar * p, QString kName, int kt, QString val, int vt){
    parent = p;
    name = kName;
    kType = kt;
    value = val;
    vType = vt;
    hidden = false;
    reference = false;
    vpointer = 0;
    kpointer = 0;
    saved = false;
}

void TVar::setReference(bool s){
    reference = s;
}

void TVar::addChild(TVar * c){
    children.append(c);
}

QString TVar::getName(){
    return name;
}

bool TVarLessThan( TVar* var, TVar* var2 ){
    QString a = var->getName();
    QString b = var2->getName();
    if ( a.toInt() && b.toInt() )
        return a.toInt() < b.toInt();
    return a.toLower() < b.toLower();
}

QList<TVar *> TVar::getChildren(int sort=1){
    if (sort)
        qSort(children.begin(), children.end(), TVarLessThan);
    return children;
}

bool TVar::isReference(){
    return reference;
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

QString TVar::getValue(){
    return value;
}

int TVar::getValueType(){
    return vType;
}

void TVar::setNewName(QString n){
    nName = n;
}

void TVar::setNewName(QString n, int t){
    nName = n;
    nkType = t;
}

int TVar::getNewKeyType(){
    return nkType;
}

QString TVar::getNewName(){
    return nName;
}

void TVar::clearNewName(){
    name = nName;
    kType = nkType;
    nName = "";
    nkType = 0;
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
    return true;
}

bool TVar::setName(QString n){
    name = n;
    return true;
}

TVar * TVar::getParent(){
    return parent;
}
