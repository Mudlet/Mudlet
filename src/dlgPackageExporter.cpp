/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017-2018 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "dlgPackageExporter.h"
#include "ui_dlgPackageExporter.h"

#include "mudlet.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

#include "pre_guard.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <zip.h>
#include "post_guard.h"


using namespace std;

dlgPackageExporter::dlgPackageExporter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dlgPackageExporter)
, treeWidget()
, exportButton()
, closeButton()
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

    mpTriggers = new QTreeWidgetItem({tr("Triggers")});
    mpAliases = new QTreeWidgetItem({tr("Aliases")});
    mpTimers = new QTreeWidgetItem({tr("Timers")});
    mpScripts = new QTreeWidgetItem({tr("Scripts")});
    mpKeys = new QTreeWidgetItem({tr("Keys")});
    mpButtons = new QTreeWidgetItem({tr("Buttons")});

    treeWidget->addTopLevelItem(mpTriggers);
    treeWidget->addTopLevelItem(mpAliases);
    treeWidget->addTopLevelItem(mpTimers);
    treeWidget->addTopLevelItem(mpScripts);
    treeWidget->addTopLevelItem(mpKeys);
    treeWidget->addTopLevelItem(mpButtons);

    closeButton = ui->buttonBox->addButton(QDialogButtonBox::Close);
    exportButton = new QPushButton(tr("&Export"));

    ui->browseButton->hide();
    ui->filePath->hide();
    ui->textLabel1->hide();

    // reset zipFile and filePath from possible previous use
    zipFile = filePath = QLatin1String("");

    packageName = QInputDialog::getText(nullptr, tr("Package name"), tr("Package name:"));
    if (packageName.isEmpty()) {
        return;
    }
    QString packagePath = QFileDialog::getExistingDirectory(nullptr, tr("Where do you want to save the package?"), tr("Where do you want to save the package?"));
    if (packagePath.isEmpty()) {
        return;
    }
    packagePath.replace(QLatin1String(R"(\)"), QLatin1String("/"));

    tempDir = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), QStringLiteral("tmp/%1").arg(packageName));
    QDir packageDir = QDir(tempDir);
    if (!packageDir.exists()) {
        packageDir.mkpath(tempDir);
    }
    zipFile = packagePath + "/" + packageName + ".zip";
    filePath = tempDir + "/" + packageName + ".xml";

    QString luaConfig = tempDir + "/config.lua";
    QFile configFile(luaConfig);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << R"(mpackage = ")" << packageName << "\"\n";
        out.flush();
        configFile.close();
    }
    connect(ui->addFiles, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_addFiles);

    ui->buttonBox->addButton(exportButton, QDialogButtonBox::ResetRole);
    connect(exportButton, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_export_package);

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

void dlgPackageExporter::recurseTree(QTreeWidgetItem* pItem, QList<QTreeWidgetItem*>& treeList)
{
    treeList.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        recurseTree(pItem->child(i), treeList);
    }
}


void dlgPackageExporter::slot_export_package()
{
    QFile checkWriteability(filePath);
    if (!checkWriteability.open(QIODevice::WriteOnly)) {
        ui->infoLabel->setText(tr("Failed to export - couldn't open %1 for writing in. Do you have the necessary permissions to write to that folder?").arg(filePath));
        return;
    }
    checkWriteability.close();

    XMLexport writer(mpHost);
    //write trigs
    QTreeWidgetItem* top = mpTriggers;
    QList<QTreeWidgetItem*> trigList;
    recurseTree(top, trigList);
    for (auto item : qAsConst(trigList)) {
        if (item->checkState(0) == Qt::Unchecked && triggerMap.contains(item)) {
            triggerMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && triggerMap.contains(item) && triggerMap[item]->mModuleMasterFolder) {
            triggerMap[item]->mModuleMasterFolder = false;
            modTriggerMap.insert(item, triggerMap[item]);
        }
    }
    top = mpTimers;
    QList<QTreeWidgetItem*> timerList;
    recurseTree(top, timerList);
    for (auto item : qAsConst(timerList)) {
        if (item->checkState(0) == Qt::Unchecked && timerMap.contains(item)) {
            timerMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && timerMap.contains(item) && timerMap[item]->mModuleMasterFolder) {
            timerMap[item]->mModuleMasterFolder = false;
            modTimerMap.insert(item, timerMap[item]);
        }
    }
    top = mpAliases;
    QList<QTreeWidgetItem*> aliasList;
    recurseTree(top, aliasList);
    for (auto item : qAsConst(aliasList)) {
        if (item->checkState(0) == Qt::Unchecked && aliasMap.contains(item)) {
            aliasMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && aliasMap.contains(item) && aliasMap[item]->mModuleMasterFolder) {
            aliasMap[item]->mModuleMasterFolder = false;
            modAliasMap.insert(item, aliasMap[item]);
        }
    }
    top = mpButtons;
    QList<QTreeWidgetItem*> actionList;
    recurseTree(top, actionList);
    for (auto item : qAsConst(actionList)) {
        if (item->checkState(0) == Qt::Unchecked && actionMap.contains(item)) {
            actionMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && actionMap.contains(item) && actionMap[item]->mModuleMasterFolder) {
            actionMap[item]->mModuleMasterFolder = false;
            modActionMap.insert(item, actionMap[item]);
        }
    }
    top = mpScripts;
    QList<QTreeWidgetItem*> scriptList;
    recurseTree(top, scriptList);
    for (auto item : qAsConst(scriptList)) {
        if (item->checkState(0) == Qt::Unchecked && scriptMap.contains(item)) {
            scriptMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && scriptMap.contains(item) && scriptMap[item]->mModuleMasterFolder) {
            scriptMap[item]->mModuleMasterFolder = false;
            modScriptMap.insert(item, scriptMap[item]);
        }
    }
    top = mpKeys;
    QList<QTreeWidgetItem*> keyList;
    recurseTree(top, keyList);
    for (auto item : qAsConst(keyList)) {
        if (item->checkState(0) == Qt::Unchecked && keyMap.contains(item)) {
            keyMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && keyMap.contains(item) && keyMap[item]->mModuleMasterFolder) {
            keyMap[item]->mModuleMasterFolder = false;
            modKeyMap.insert(item, keyMap[item]);
        }
    }

    writer.exportPackage(filePath);

    //now fix all the stuff we weren't exporting
    //trigger, timer, alias,action,script, keys
    for (auto item : qAsConst(trigList)) {
        if (triggerMap.contains(item)) {
            triggerMap[item]->exportItem = true;
        }
        if (modTriggerMap.contains(item)) {
            modTriggerMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(timerList)) {
        if (timerMap.contains(item)) {
            timerMap[item]->exportItem = true;
        }
        if (modTimerMap.contains(item)) {
            modTimerMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(actionList)) {
        if (actionMap.contains(item)) {
            actionMap[item]->exportItem = true;
        }
        if (modActionMap.contains(item)) {
            modActionMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(scriptList)) {
        if (scriptMap.contains(item)) {
            scriptMap[item]->exportItem = true;
        }
        if (modScriptMap.contains(item)) {
            modScriptMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(keyList)) {
        if (keyMap.contains(item)) {
            keyMap[item]->exportItem = true;
        }
        if (modKeyMap.contains(item)) {
            modKeyMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(aliasList)) {
        if (aliasMap.contains(item)) {
            aliasMap[item]->exportItem = true;
        }
        if (modAliasMap.contains(item)) {
            modAliasMap[item]->mModuleMasterFolder = true;
        }
    }


    int err = 0;
    char buf[100];
    zip* archive;
    archive = zip_open(zipFile.toStdString().c_str(), ZIP_CREATE, &err);
    if (err != 0) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        //FIXME: report error to user qDebug()<<"dp zip open error"<<zipFile<<buf;
        close();
        return;
    }
    QDir dir(tempDir);
    QStringList contents = dir.entryList();
    for (int i = 0; i < contents.size(); i++) {
        QString fname = contents[i];
        if (fname == "." || fname == "..") {
            continue;
        }
        QString fullName = tempDir + "/" + contents[i];
        struct zip_source* s = zip_source_file(archive, fullName.toStdString().c_str(), 0, 0);
        if (s == nullptr) {
            int sep = 0;
            zip_error_get(archive, &err, &sep);
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: report error to userqDebug()<<"zip source error"<<fullName<<fname<<buf;
        }
        err = zip_file_add(archive, fname.toStdString().c_str(), s, ZIP_FL_OVERWRITE);
        if (err == -1) {
            int sep = 0;
            zip_error_get(archive, &err, &sep);
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: report error to userqDebug()<<"added file error"<<fullName<<fname<<buf;
        }
    }
    err = zip_close(archive);
    if (err != 0) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        //FIXME: report error to userqDebug()<<"dp close file error"<<buf;
        close();
        return;
    }

    showUploadNudge();

}

void dlgPackageExporter::showUploadNudge()
{
   ui->infoLabel->setText(R"(<a href="https://forums.mudlet.org/posting.php?mode=post&f=6">Upload package to Mudlet</a>)");
   ui->infoLabel->setTextFormat(Qt::RichText);
   ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
   ui->infoLabel->setOpenExternalLinks(true);
}

void dlgPackageExporter::slot_addFiles()
{
    QString _pn = "file:///" + tempDir;
    QDesktopServices::openUrl(QUrl(_pn, QUrl::TolerantMode));
}

void dlgPackageExporter::recurseTriggers(TTrigger* trig, QTreeWidgetItem* qTrig)
{
    list<TTrigger*>* childList = trig->getChildrenList();
    if (childList->empty()) {
        return;
    }
    list<TTrigger*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TTrigger* pChild = *it;
        if (pChild->isTemporary())
            continue;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        triggerMap.insert(pItem, pChild);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qTrig->addChild(pItem);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::listTriggers()
{
    TriggerUnit* tu = mpHost->getTriggerUnit();
    list<TTrigger*>::const_iterator it;
    std::list<TTrigger*> tList = tu->getTriggerRootNodeList();
    QTreeWidgetItem* top = mpTriggers;
    for (it = tList.begin(); it != tList.end(); it++) {
        TTrigger* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        top->addChild(pItem);
        triggerMap.insert(pItem, pChild);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::recurseAliases(TAlias* item, QTreeWidgetItem* qItem)
{
    list<TAlias*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    list<TAlias*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TAlias* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qItem->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::listAliases()
{
    AliasUnit* tu = mpHost->getAliasUnit();
    list<TAlias*>::const_iterator it;
    std::list<TAlias*> tList = tu->getAliasRootNodeList();
    QTreeWidgetItem* top = mpAliases;
    for (it = tList.begin(); it != tList.end(); it++) {
        TAlias* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        top->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::recurseScripts(TScript* item, QTreeWidgetItem* qItem)
{
    list<TScript*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    list<TScript*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TScript* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::listScripts()
{
    ScriptUnit* tu = mpHost->getScriptUnit();
    list<TScript*>::const_iterator it;
    std::list<TScript*> tList = tu->getScriptRootNodeList();
    QTreeWidgetItem* top = mpScripts;
    for (it = tList.begin(); it != tList.end(); it++) {
        TScript* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::recurseKeys(TKey* item, QTreeWidgetItem* qItem)
{
    list<TKey*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    list<TKey*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TKey* pChild = *it;
        if (pChild->isTemporary())
            continue;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::listKeys()
{
    KeyUnit* tu = mpHost->getKeyUnit();
    list<TKey*>::const_iterator it;
    std::list<TKey*> tList = tu->getKeyRootNodeList();
    QTreeWidgetItem* top = mpKeys;
    for (it = tList.begin(); it != tList.end(); it++) {
        TKey* pChild = *it;
        if (pChild->isTemporary())
            continue;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::recurseActions(TAction* item, QTreeWidgetItem* qItem)
{
    list<TAction*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    list<TAction*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TAction* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::listActions()
{
    ActionUnit* tu = mpHost->getActionUnit();
    list<TAction*>::const_iterator it;
    std::list<TAction*> tList = tu->getActionRootNodeList();
    QTreeWidgetItem* top = mpButtons;
    for (it = tList.begin(); it != tList.end(); it++) {
        TAction* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::recurseTimers(TTimer* item, QTreeWidgetItem* qItem)
{
    list<TTimer*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    list<TTimer*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TTimer* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::listTimers()
{
    TimerUnit* tu = mpHost->getTimerUnit();
    list<TTimer*>::const_iterator it;
    std::list<TTimer*> tList = tu->getTimerRootNodeList();
    QTreeWidgetItem* top = mpTimers;
    for (it = tList.begin(); it != tList.end(); it++) {
        TTimer* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}
