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
    void setBase(TVar *);
    TVar * getBase();
    void clear();
    void buildVarTree( QTreeWidgetItem *, TVar * );
private:
    QSet< TVar * > vars;
    TVar * base;
    QSet< QStringList * > varList;
};

#endif // VARUNIT_H
