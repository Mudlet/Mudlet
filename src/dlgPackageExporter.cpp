#include "dlgPackageExporter.h"
#include "ui_dlgPackageExporter.h"
#include "Host.h"
#include <QFileDialog>
#include <QInputDialog>
#include "XMLexport.h"
#include <QDesktopServices>
#include "zip.h"
#include "zipconf.h"
#include <errno.h>

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

    closeButton = ui->buttonBox->addButton (QDialogButtonBox::Close);
    exportButton = new QPushButton(tr("&Export"));

    ui->browseButton->hide();
    ui->filePath->hide();
    ui->textLabel1->hide();

    // reset zipFile and filePath from possible previous use
    zipFile = filePath = "";

    packageName = QInputDialog::getText(0,"Package name", "Package name:");
    if (packageName.isEmpty()) {
        return;
    }
    QString packagePath = QFileDialog::getExistingDirectory(0,"Where do you want to save the package?","Where do you want to save the package?");
    if (packagePath.isEmpty()) {
        return;
    }

    tempDir = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName()+"/tmp/";
    packagePath.replace("\\", "/");
    tempDir = tempDir + "/" + packageName;
    QDir packageDir = QDir(tempDir);
    if ( !packageDir.exists() ){
        packageDir.mkpath(tempDir);
    }
    zipFile = packagePath + "/" + packageName + ".zip";
    filePath = tempDir + "/" + packageName + ".xml";

    QString luaConfig = tempDir + "/config.lua";
    QFile configFile(luaConfig);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&configFile);
        out << "mpackage = \"" << packageName << "\"\n";
        out.flush();
        configFile.close();
    }
    connect(ui->addFiles, SIGNAL(clicked()), this, SLOT(slot_addFiles()));

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
//#ifndef Q_OS_WIN
//    filePath = ui->filePath->text();
//#endif
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
            else if (item->checkState(0) == Qt::Checked && triggerMap.contains(item) && triggerMap[item]->mModuleMasterFolder){
                triggerMap[item]->mModuleMasterFolder=false;
                modTriggerMap.insert(item, triggerMap[item]);
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
            else if (item->checkState(0) == Qt::Checked && timerMap.contains(item) && timerMap[item]->mModuleMasterFolder){
                timerMap[item]->mModuleMasterFolder=false;
                modTimerMap.insert(item, timerMap[item]);
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
            else if (item->checkState(0) == Qt::Checked && aliasMap.contains(item) && aliasMap[item]->mModuleMasterFolder){
                aliasMap[item]->mModuleMasterFolder=false;
                modAliasMap.insert(item, aliasMap[item]);
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
            else if (item->checkState(0) == Qt::Checked && actionMap.contains(item) && actionMap[item]->mModuleMasterFolder){
                actionMap[item]->mModuleMasterFolder=false;
                modActionMap.insert(item, actionMap[item]);
            }
        }
        items = treeWidget->findItems(QString("Scripts"), Qt::MatchExactly, 0);
        top = items.first();
        QList<QTreeWidgetItem *> scriptList;
        recurseTree(top,scriptList);
        for (int i=0;i<scriptList.size();i++){
            QTreeWidgetItem * item = scriptList.at(i);
            if (item->checkState(0) == Qt::Unchecked && scriptMap.contains(item)){
                scriptMap[item]->exportItem = false;
            }
            else if (item->checkState(0) == Qt::Checked && scriptMap.contains(item) && scriptMap[item]->mModuleMasterFolder){
                scriptMap[item]->mModuleMasterFolder=false;
                modScriptMap.insert(item, scriptMap[item]);
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
            else if (item->checkState(0) == Qt::Checked && keyMap.contains(item) && keyMap[item]->mModuleMasterFolder){
                keyMap[item]->mModuleMasterFolder=false;
                modKeyMap.insert(item, keyMap[item]);
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
            if (modTriggerMap.contains(item)){
                modTriggerMap[item]->mModuleMasterFolder = true;
            }
        }
        for (int i=0;i<timerList.size();i++){
            QTreeWidgetItem * item = timerList.at(i);
            if (timerMap.contains(item)){
                timerMap[item]->exportItem = true;
            }
            if (modTimerMap.contains(item)){
                modTimerMap[item]->mModuleMasterFolder = true;
            }
        }
        for (int i=0;i<actionList.size();i++){
            QTreeWidgetItem * item = actionList.at(i);
            if (actionMap.contains(item)){
                actionMap[item]->exportItem = true;
            }
            if (modActionMap.contains(item)){
                modActionMap[item]->mModuleMasterFolder = true;
            }
        }
        for (int i=0;i<scriptList.size();i++){
            QTreeWidgetItem * item = scriptList.at(i);
            if (scriptMap.contains(item)){
                scriptMap[item]->exportItem = true;
            }
            if (modScriptMap.contains(item)){
                modScriptMap[item]->mModuleMasterFolder = true;
            }
        }
        for (int i=0;i<keyList.size();i++){
            QTreeWidgetItem * item = keyList.at(i);
            if (keyMap.contains(item)){
                keyMap[item]->exportItem = true;
            }
            if (modKeyMap.contains(item)){
                modKeyMap[item]->mModuleMasterFolder = true;
            }
        }
        for (int i=0;i<aliasList.size();i++){
            QTreeWidgetItem * item = aliasList.at(i);
            if (aliasMap.contains(item)){
                aliasMap[item]->exportItem = true;
            }
            if (modAliasMap.contains(item)){
                modAliasMap[item]->mModuleMasterFolder = true;
            }
        }


        //#ifdef Q_OS_WIN
        int err = 0;
        char buf[100];
        zip* archive;
        //archive = zip_open( zipFile.toStdString().c_str(), ZIP_CREATE|ZIP_TRUNCATE, &err);
        archive = zip_open( zipFile.toStdString().c_str(), ZIP_CREATE, &err);
        if ( err != 0 )
        {
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: report error to user qDebug()<<"dp zip open error"<<zipFile<<buf;
            close();
            return;
        }
        QDir dir(tempDir);
        QStringList contents = dir.entryList();
        for(int i=0;i<contents.size();i++)
        {
            QString fname = contents[i];
            if ( fname == "." || fname == ".." )
                continue;
            QString fullName = tempDir+"/"+contents[i];
            struct zip_source *s = zip_source_file( archive, fullName.toStdString().c_str(), 0, 0);
            if ( s == NULL )
            {
                int sep = 0;
                zip_error_get( archive, &err, &sep);
                zip_error_to_str(buf, sizeof(buf), err, errno);
                //FIXME: report error to userqDebug()<<"zip source error"<<fullName<<fname<<buf;
            }
//            err = zip_file_add( archive, fname.toStdString().c_str(), s, ZIP_FL_OVERWRITE );
            err = zip_add( archive, fname.toStdString().c_str(), s);
            if ( err == -1 )
            {
                int sep = 0;
                zip_error_get( archive, &err, &sep);
                zip_error_to_str(buf, sizeof(buf), err, errno);
                //FIXME: report error to userqDebug()<<"added file error"<<fullName<<fname<<buf;
            }
        }
        err = zip_close( archive );
        if ( err != 0 ){
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: report error to userqDebug()<<"dp close file error"<<buf;
            close();
            return;
        }
            //JlCompress::compressDir(zip, tempDir );
//        #else
//            ui->infoLabel->setText("Exported package to "+filePath);
//        #endif
    } else {
        ui->infoLabel->setText("Failed to export - couldn't open "+filePath+" for writing in. Do you have the necessary permissions to write to that folder?");
    }
    close();
}

void dlgPackageExporter::slot_addFiles(){
    QString _pn = "file:///"+tempDir;
    QDesktopServices::openUrl(QUrl(_pn, QUrl::TolerantMode));
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
        if( pChild->isTempTrigger() ) continue;
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
        if (pChild->isTempAlias())
            continue;
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
        if (pChild->isTempTimer())
            continue;
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
