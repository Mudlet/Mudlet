#ifndef VARUNIT_H
#define VARUNIT_H

#include <QMap>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QSet>
#include "TVar.h"

class VarUnit
{
public:
    VarUnit();
    QStringList * varName(TVar * var);
    QStringList shortVarName(TVar * var);
    bool varExists(TVar *);
    void addVariable(TVar *);
    void addTempVar( QTreeWidgetItem * , TVar * );
    void removeTempVar( QTreeWidgetItem * );
    void removeVariable(TVar *);
    void setBase(TVar *);
    TVar * getBase();
    void clear();
    void clearTemp();
    void buildVarTree( QTreeWidgetItem *, TVar * );
    TVar * getWVar( QTreeWidgetItem * );
    TVar * getTVar( QTreeWidgetItem * );
    void addTreeItem( QTreeWidgetItem *, TVar * );
    void addHidden( TVar * var );
    bool isHidden( TVar * var );
public:
    QSet< QString > hidden;
private:
    TVar * base;
    QSet< QStringList * > varList;
    QMap< QTreeWidgetItem *, TVar * > wVars;
    QMap< QTreeWidgetItem *, TVar * > tVars;
};

#endif // VARUNIT_H
