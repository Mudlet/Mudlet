#ifndef MUDLET_VARUNIT_H
#define MUDLET_VARUNIT_H

/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "pre_guard.h"
#include <QMap>
#include <QSet>
#include <QStringList>
#include "post_guard.h"


class TVar;

class QTreeWidgetItem;


class VarUnit
{
public:
    VarUnit();
    QStringList varName(TVar*);
    QStringList shortVarName(TVar*);
    bool varExists(TVar*);
    bool shouldSave(QTreeWidgetItem*);
    bool shouldSave(TVar*);
    void addVariable(TVar*);
    void addTempVar(QTreeWidgetItem*, TVar*);
    void removeTempVar(QTreeWidgetItem*);
    void removeVariable(TVar*);
    void setBase(TVar*);
    TVar* getBase();
    void clear();
    void buildVarTree(QTreeWidgetItem*, TVar*, bool);
    TVar* getWVar(QTreeWidgetItem*);
    TVar* getTVar(QTreeWidgetItem*);
    void addTreeItem(QTreeWidgetItem*, TVar*);
    void addSavedVar(TVar*);
    void removeSavedVar(TVar*);
    void addHidden(TVar*, int);
    void addHidden(QString);
    bool isHidden(TVar *var);
    bool isHidden(const QString &fullname);
    void removeHidden(TVar *var);
    void removeHidden(const QString &name);
    bool isSaved(TVar*);
    void addPointer(const void*);
    QSet<QString> hidden;
    QSet<QString> hiddenByUser;
    QSet<QString> savedVars;

private:
    TVar* base;
    QSet<QString> varList;
    QMap<QTreeWidgetItem*, TVar*> wVars;
    QMap<QTreeWidgetItem*, TVar*> tVars;
    QSet<const void*> pointers;
};

#endif // MUDLET_VARUNIT_H
