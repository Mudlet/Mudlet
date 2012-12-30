#include "dlgPackageExporter.h"
#include "ui_dlgPackageExporter.h"
#include "Host.h"
#include <QFileDialog>
#include "XMLexport.h"

dlgPackageExporter::dlgPackageExporter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dlgPackageExporter)
{
    ui->setupUi(this);
}

dlgPackageExporter::dlgPackageExporter(QWidget *parent, Host* host) :
    QDialog(parent),
    ui(new Ui::dlgPackageExporter)
{
    mpHost = host;
    ui->setupUi(this);
    treeWidget = ui->treeWidget;
    connect(ui->browseButton, SIGNAL(clicked()), this, SLOT(slot_browse_button()));

    closeButton = ui->buttonBox->addButton (QDialogButtonBox::Close);

    exportButton = new QPushButton(tr("&Export"));
    exportButton->setDisabled(true); // disabled by default until the user selects a location
    ui->buttonBox->addButton(exportButton, QDialogButtonBox::ResetRole);
    connect(exportButton, SIGNAL(clicked()), this, SLOT(slot_export_package()));

    listTriggers();
    listAliases();
    listKeys();
    listScripts();
    listActions();
    listTimers();
}

dlgPackageExporter::~dlgPackageExporter()
{
    delete ui;
}

void dlgPackageExporter::recurseTree(QTreeWidgetItem * pItem, QList<QTreeWidgetItem *> &treeList){
    treeList.append(pItem);
    for(int i=0;i<pItem->childCount();++i)
        recurseTree(pItem->child(i), treeList);
}

void dlgPackageExporter::slot_export_package(){
    QString filePath = ui->filePath->text();
    QFile file_xml( filePath );
    if( file_xml.open( QIODevice::WriteOnly ) )
    {
        XMLexport writer( mpHost );
        //write trigs
        QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Triggers"), Qt::MatchExactly, 0);
        QTreeWidgetItem * top = items.first();
        QList<QTreeWidgetItem *> trigList;
        recurseTree(top,trigList);
        for (int i=0;i<trigList.size();i++){
            QTreeWidgetItem * item = trigList.at(i);
            if (item->checkState(0) == Qt::Unchecked && triggerMap.contains(item)){
                triggerMap[item]->exportItem = false;
            }
        }
        items = treeWidget->findItems(QString("Timers"), Qt::MatchExactly, 0);
        top = items.first();
        QList<QTreeWidgetItem *> timerList;
        recurseTree(top,timerList);
        for (int i=0;i<timerList.size();i++){
            QTreeWidgetItem * item = timerList.at(i);
            if (item->checkState(0) == Qt::Unchecked && timerMap.contains(item)){
                timerMap[item]->exportItem = false;
            }
        }
        items = treeWidget->findItems(QString("Aliases"), Qt::MatchExactly, 0);
        top = items.first();
        QList<QTreeWidgetItem *> aliasList;
        recurseTree(top,aliasList);
        for (int i=0;i<aliasList.size();i++){
            QTreeWidgetItem * item = aliasList.at(i);
            if (item->checkState(0) == Qt::Unchecked && aliasMap.contains(item)){
                aliasMap[item]->exportItem = false;
            }
        }
        items = treeWidget->findItems(QString("Buttons"), Qt::MatchExactly, 0);
        top = items.first();
        QList<QTreeWidgetItem *> actionList;
        recurseTree(top,actionList);
        for (int i=0;i<actionList.size();i++){
            QTreeWidgetItem * item = actionList.at(i);
            if (item->checkState(0) == Qt::Unchecked && actionMap.contains(item)){
                actionMap[item]->exportItem = false;
            }
        }
        items = treeWidget->findItems(QString("Scripts"), Qt::MatchExactly, 0);
        top = items.first();
        QList<QTreeWidgetItem *> scriptList;
        recurseTree(top,scriptList);
        for (int i=0;i<scriptList.size();i++){
            QTreeWidgetItem * item = scriptList.at(i);
            if (item->checkState(0) == Qt::Unchecked && scriptMap.contains(item)){
                qDebug()<<"not writing"<<scriptMap[item]->getName();
                scriptMap[item]->exportItem = false;
            }
        }
        items = treeWidget->findItems(QString("Keys"), Qt::MatchExactly, 0);
        top = items.first();
        QList<QTreeWidgetItem *> keyList;
        recurseTree(top,keyList);
        for (int i=0;i<keyList.size();i++){
            QTreeWidgetItem * item = keyList.at(i);
            if (item->checkState(0) == Qt::Unchecked && keyMap.contains(item)){
                keyMap[item]->exportItem = false;
            }
        }
        writer.exportGenericPackage(&file_xml);
        file_xml.close();
        //now fix all the stuff we weren't exporting
        //trigger, timer, alias,action,script, keys
        for (int i=0;i<trigList.size();i++){
            QTreeWidgetItem * item = trigList.at(i);
            if (triggerMap.contains(item)){
                triggerMap[item]->exportItem = true;
            }
        }
        for (int i=0;i<timerList.size();i++){
            QTreeWidgetItem * item = timerList.at(i);
            if (timerMap.contains(item)){
                timerMap[item]->exportItem = true;
            }
        }
        for (int i=0;i<actionList.size();i++){
            QTreeWidgetItem * item = actionList.at(i);
            if (actionMap.contains(item)){
                actionMap[item]->exportItem = true;
            }
        }
        for (int i=0;i<scriptList.size();i++){
            QTreeWidgetItem * item = scriptList.at(i);
            if (scriptMap.contains(item)){
                scriptMap[item]->exportItem = true;
            }
        }
        for (int i=0;i<keyList.size();i++){
            QTreeWidgetItem * item = keyList.at(i);
            if (keyMap.contains(item)){
                keyMap[item]->exportItem = true;
            }
        }
        for (int i=0;i<aliasList.size();i++){
            QTreeWidgetItem * item = aliasList.at(i);
            if (aliasMap.contains(item)){
                aliasMap[item]->exportItem = true;
            }
        }

        ui->infoLabel->setText("Exported package to "+filePath);
    } else {
        ui->infoLabel->setText("Failed to export - couldn't open "+filePath+" for writing in. Do you have the necessary permissions to write to that folder?");
    }
}

void dlgPackageExporter::slot_browse_button(){
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Mudlet Packages (*.xml)"));
    dialog.setViewMode(QFileDialog::Detail);
    QString fileName;
    if (dialog.exec()) {
        fileName = dialog.selectedFiles().first();

        if (!fileName.endsWith(".xml"))
            fileName.append(".xml");

        ui->filePath->setText(fileName);
        exportButton->setDisabled(false);
    }
}

void dlgPackageExporter::recurseTriggers(TTrigger* trig, QTreeWidgetItem* qTrig){
    list<TTrigger *> * childList = trig->getChildrenList();
    if (!childList->size())
        return;
    list<TTrigger *>::iterator it;
    for(it=childList->begin(); it!=childList->end();it++){
        TTrigger * pChild = *it;
        if (pChild->isTempTrigger())
            continue;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        triggerMap.insert(pItem, pChild);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qTrig->addChild(pItem);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::listTriggers()
{
    TriggerUnit* tu = mpHost->getTriggerUnit();
    list<TTrigger *>::const_iterator it;
    std::list<TTrigger *> tList = tu->getTriggerRootNodeList();
    QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Triggers"), Qt::MatchExactly, 0);
    QTreeWidgetItem * top = items.first();
    for(it = tList.begin(); it != tList.end(); it++)
    {
        TTrigger * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        top->addChild(pItem);
        triggerMap.insert(pItem, pChild);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::recurseAliases(TAlias* item, QTreeWidgetItem* qItem){
    list<TAlias *> * childList = item->getChildrenList();
    if (!childList->size())
        return;
    list<TAlias *>::iterator it;
    for(it=childList->begin(); it!=childList->end();it++){
        TAlias * pChild = *it;
        if (pChild->isTempAlias())
            continue;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qItem->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::listAliases()
{
    AliasUnit* tu = mpHost->getAliasUnit();
    list<TAlias *>::const_iterator it;
    std::list<TAlias *> tList = tu->getAliasRootNodeList();
    QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Aliases"), Qt::MatchExactly, 0);
    QTreeWidgetItem * top = items.first();
    for(it = tList.begin(); it != tList.end(); it++)
    {
        TAlias * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        top->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::recurseScripts(TScript* item, QTreeWidgetItem* qItem){
    list<TScript *> * childList = item->getChildrenList();
    if (!childList->size())
        return;
    list<TScript *>::iterator it;
    for(it=childList->begin(); it!=childList->end();it++){
        TScript * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::listScripts()
{
    ScriptUnit* tu = mpHost->getScriptUnit();
    list<TScript *>::const_iterator it;
    std::list<TScript *> tList = tu->getScriptRootNodeList();
    QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Scripts"), Qt::MatchExactly, 0);
    QTreeWidgetItem * top = items.first();
    for(it = tList.begin(); it != tList.end(); it++)
    {
        TScript * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::recurseKeys(TKey* item, QTreeWidgetItem* qItem){
    list<TKey *> * childList = item->getChildrenList();
    if (!childList->size())
        return;
    list<TKey *>::iterator it;
    for(it=childList->begin(); it!=childList->end();it++){
        TKey * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::listKeys()
{
    KeyUnit* tu = mpHost->getKeyUnit();
    list<TKey *>::const_iterator it;
    std::list<TKey *> tList = tu->getKeyRootNodeList();
    QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Keys"), Qt::MatchExactly, 0);
    QTreeWidgetItem * top = items.first();
    for(it = tList.begin(); it != tList.end(); it++)
    {
        TKey * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::recurseActions(TAction* item, QTreeWidgetItem* qItem){
    list<TAction *> * childList = item->getChildrenList();
    if (!childList->size())
        return;
    list<TAction *>::iterator it;
    for(it=childList->begin(); it!=childList->end();it++){
        TAction * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::listActions()
{
    ActionUnit* tu = mpHost->getActionUnit();
    list<TAction *>::const_iterator it;
    std::list<TAction *> tList = tu->getActionRootNodeList();
    QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Buttons"), Qt::MatchExactly, 0);
    QTreeWidgetItem * top = items.first();
    for(it = tList.begin(); it != tList.end(); it++)
    {
        TAction * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::recurseTimers(TTimer* item, QTreeWidgetItem* qItem){
    list<TTimer *> * childList = item->getChildrenList();
    if (!childList->size())
        return;
    list<TTimer *>::iterator it;
    for(it=childList->begin(); it!=childList->end();it++){
        TTimer * pChild = *it;
        if (pChild->isTempTimer())
            continue;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::listTimers()
{
    TimerUnit* tu = mpHost->getTimerUnit();
    list<TTimer *>::const_iterator it;
    std::list<TTimer *> tList = tu->getTimerRootNodeList();
    QList<QTreeWidgetItem *> items = treeWidget->findItems(QString("Timers"), Qt::MatchExactly, 0);
    QTreeWidgetItem * top = items.first();
    for(it = tList.begin(); it != tList.end(); it++)
    {
        TTimer * pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        QTreeWidgetItem * pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsTristate|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}
