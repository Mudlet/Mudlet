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

#ifndef TVAR_H
#define TVAR_H

#include <QString>
#include <QStringList>
#include <QList>

class TVar
{
public:
    TVar();
    TVar(TVar *);
    TVar(TVar *, QString, int, QString, int);
    void    addChild(TVar *);
    void    setParent(TVar *);
    void    removeChild(TVar *);
    bool    setValue(QString);
    bool    setValue(QString, int);
    bool    setValueType(int);
    bool    setKeyType(int);
    bool    setName(QString);
    bool    setName(QString, int);
    void    setNewName(QString, int);
    void    setNewName(QString);
    void    setReference(bool);
    QList<TVar *>  getChildren(int);
    TVar *  getParent();
    QString getValue();
    QString getName();
    QString getNewName();
    void clearNewName();
    int     getKeyType();
    int     getNewKeyType();
    int     getValueType();
    bool    isReference();
public:
    bool    hidden;
    const void*   kpointer;
    const void*   vpointer;
    bool    saved;
private:
    bool        reference;
    QList<TVar *> children;
    TVar * parent;
    QString     name;
    int         kType;
    QString     value;
    int         vType;
    int         nkType;
    int         nvType;
    QString     nName;
    QString     nValue;
};

#endif // TVAR_H
