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
    QStringList * varName(TVar * );
    QStringList shortVarName(TVar * );
    bool varExists(TVar *);
    void addVariable(TVar *);
    void addTempVar( QTreeWidgetItem * , TVar * );
    void removeTempVar( QTreeWidgetItem * );
    void removeVariable(TVar *);
    void setBase(TVar *);
    TVar * getBase();
    void clear();
    void clearTemp();
    void buildVarTree( QTreeWidgetItem *, TVar *, bool );
    TVar * getWVar( QTreeWidgetItem * );
    TVar * getTVar( QTreeWidgetItem * );
    void addTreeItem( QTreeWidgetItem *, TVar * );
    void addSavedVar( TVar * );
    void removeSavedVar( TVar * );
    void removeHidden( TVar * );
    void addHidden( TVar * );
    bool isHidden( TVar * );
    bool isSaved( TVar * );
public:
    QSet< QString > hidden;
private:
    TVar * base;
    QSet< QStringList * > varList;
    QMap< QTreeWidgetItem *, TVar * > wVars;
    QMap< QTreeWidgetItem *, TVar * > tVars;
    QSet< QString > savedVars;
};

#endif // VARUNIT_H
