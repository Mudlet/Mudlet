#ifndef DLGPACKAGEEXPORTER_H
#define DLGPACKAGEEXPORTER_H

#include <QDialog>
#include "Host.h"

namespace Ui {
class dlgPackageExporter;
}

class dlgPackageExporter : public QDialog
{
    Q_OBJECT

public:
    explicit dlgPackageExporter(QWidget *parent = 0);
    explicit dlgPackageExporter(QWidget *parent, Host*);
    ~dlgPackageExporter();
    void recurseTree(QTreeWidgetItem *, QList<QTreeWidgetItem *>&);
    void listTriggers();
    void recurseTriggers(TTrigger*, QTreeWidgetItem*);
    void listAliases();
    void recurseAliases(TAlias*, QTreeWidgetItem*);
    void listScripts();
    void recurseScripts(TScript*, QTreeWidgetItem*);
    void listKeys();
    void recurseKeys(TKey*, QTreeWidgetItem*);
    void listActions();
    void recurseActions(TAction*, QTreeWidgetItem*);
    void listTimers();
    void recurseTimers(TTimer*, QTreeWidgetItem*);
    QMap<QTreeWidgetItem *, TTrigger*> triggerMap;
    QMap<QTreeWidgetItem *, TAlias*> aliasMap;
    QMap<QTreeWidgetItem *, TScript*> scriptMap;
    QMap<QTreeWidgetItem *, TKey*> keyMap;
    QMap<QTreeWidgetItem *, TAction*> actionMap;
    QMap<QTreeWidgetItem *, TTimer*> timerMap;
private:
    Ui::dlgPackageExporter *ui;
    Host* mpHost;
    QTreeWidget * treeWidget;
    QPushButton *exportButton;
    QPushButton *closeButton;
public slots:
    void slot_browse_button();
    void slot_export_package();
};

#endif // DLGPACKAGEEXPORTER_H
