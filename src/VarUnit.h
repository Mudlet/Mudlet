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
    bool varExists(TVar *);
    void addVariable(TVar *);
    void removeVariable(TVar *);
    void setBase(TVar *);
    TVar * getBase();
    void clear();
    void buildVarTree( QTreeWidgetItem *, TVar * );
    TVar * getWVar( QTreeWidgetItem * );
private:
    TVar * base;
    QSet< QStringList * > varList;
    QMap< QTreeWidgetItem *, TVar * > wVars;
};

#endif // VARUNIT_H
