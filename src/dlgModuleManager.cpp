/***************************************************************************
 *   Copyright (C) 2011 by Chris Mitchell                                  *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2021-2022 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "dlgModuleManager.h"

#include "mudlet.h"

#include "pre_guard.h"
#include <QFileDialog>
#include <QMessageBox>
#include "post_guard.h"


dlgModuleManager::dlgModuleManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, mpHost(pHost)
{
    setupUi(this);

    layoutModules();
    connect(uninstallButton, &QAbstractButton::clicked, this, &dlgModuleManager::slot_uninstallModule);
    connect(installButton, &QAbstractButton::clicked, this, &dlgModuleManager::slot_installModule);
    connect(helpButton, &QAbstractButton::clicked, this, &dlgModuleManager::slot_helpModule);
    connect(moduleTable, &QTableWidget::itemClicked, this, &dlgModuleManager::slot_moduleClicked);
    connect(moduleTable, &QTableWidget::itemChanged, this, &dlgModuleManager::slot_moduleChanged);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgModuleManager::close);
    setWindowTitle(tr("Module Manager - %1").arg(mpHost->getName()));
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgModuleManager::~dlgModuleManager()
{
}

void dlgModuleManager::layoutModules()
{
    if (!mpHost) {
        return;
    }

    QMapIterator<QString, QStringList> it(mpHost->mInstalledModules);
    QStringList sl;
    sl << tr("Module Name") << tr("Priority") << tr("Sync") << tr("Module Location");
    moduleTable->setHorizontalHeaderLabels(sl);
    moduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    moduleTable->verticalHeader()->hide();
    moduleTable->setShowGrid(true);
    //clear everything
    for (int i = 0; i <= moduleTable->rowCount(); i++) {
        moduleTable->removeRow(i);
    }
    //order modules by priority and then alphabetically
    QMap<int, QStringList> mOrder;
    while (it.hasNext()) {
        it.next();
        const int priority = mpHost->mModulePriorities[it.key()];
        if (mOrder.contains(priority)) {
            mOrder[priority].append(it.key());
        } else {
            mOrder[priority] = QStringList(it.key());
        }
    }
    QMapIterator<int, QStringList> it2(mOrder);
    while (it2.hasNext()) {
        it2.next();
        QStringList pModules = it2.value();
        pModules.sort();
        for (int i = 0; i < pModules.size(); i++) {
            const int row = moduleTable->rowCount();
            moduleTable->insertRow(row);
            auto masterModule = new QTableWidgetItem();
            auto itemEntry = new QTableWidgetItem();
            auto itemLocation = new QTableWidgetItem();
            auto itemPriority = new QTableWidgetItem();
            QStringList moduleInfo = mpHost->mInstalledModules[pModules[i]];

            if (moduleInfo.at(1).toInt()) {
                masterModule->setCheckState(Qt::Checked);
            } else {
                masterModule->setCheckState(Qt::Unchecked);
            }
            masterModule->setText(QString());
            masterModule->setToolTip(utils::richText(tr("Checking this box will cause the module to be saved and <i>resynchronised</i> across all "
                                                        "sessions that share it when the <i>Save Profile</i> button is clicked in the Editor or if it "
                                                        "is saved at the end of the session.")));

            // Although there is now no text used here this may help to make the
            // checkbox more central in the column
            masterModule->setTextAlignment(Qt::AlignCenter);

            const QString moduleName = pModules[i];
            itemEntry->setText(moduleName);
            itemEntry->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            itemLocation->setText(moduleInfo[0]);
            itemLocation->setToolTip(utils::richText(moduleInfo[0]));     // show the full path in a tooltip, in case it doesn't fit in the table
            itemLocation->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); // disallow editing of module path, because that is not saved
            itemPriority->setData(Qt::EditRole, mpHost->mModulePriorities[moduleName]);
            moduleTable->setItem(row, 0, itemEntry);
            moduleTable->setItem(row, 1, itemPriority);
            moduleTable->setItem(row, 2, masterModule);
            moduleTable->setItem(row, 3, itemLocation);
        }
    }
    moduleTable->resizeColumnsToContents();
}

void dlgModuleManager::slot_installModule()
{
    if (!mpHost) {
        return;
    }

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Load Mudlet Module"), lastDir);
    if (fileName.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Load Mudlet Module:"), tr("Cannot read file %1:\n%2.").arg(fileName.toHtmlEscaped(), file.errorString()));
        return;
    }

    mpHost->installPackage(fileName, 1);
    for (int i = moduleTable->rowCount() - 1; i >= 0; --i) {
        moduleTable->removeRow(i);
    }

    layoutModules();
}

void dlgModuleManager::slot_uninstallModule()
{
    if (!mpHost) {
        return;
    }

    const int cRow = moduleTable->currentRow();
    QTableWidgetItem* pI = moduleTable->item(cRow, 0);
    if (pI) {
        mpHost->uninstallPackage(pI->text(), 1);
    }
    for (int i = moduleTable->rowCount() - 1; i >= 0; --i) {
        moduleTable->removeRow(i);
    }
    layoutModules();
}

void dlgModuleManager::slot_moduleClicked(QTableWidgetItem* pItem)
{
    if (!mpHost) {
        return;
    }

    const int i = pItem->row();

    QTableWidgetItem* entry = moduleTable->item(i, 0);
    QTableWidgetItem* checkStatus = moduleTable->item(i, 2);
    QTableWidgetItem* itemPriority = moduleTable->item(i, 1);
    //  Not used programmatically now: QTableWidgetItem* itemPath = moduleTable->item(i, 3);
    if (!entry || !checkStatus || !itemPriority || !mpHost->mInstalledModules.contains(entry->text())) {
        helpButton->setDisabled(true);
        if (checkStatus) {
            checkStatus->setCheckState(Qt::Unchecked);
            checkStatus->setFlags(Qt::NoItemFlags);
        }
        return;
    }

    if (mpHost->moduleHelp.contains(entry->text())) {
        helpButton->setDisabled((!mpHost->moduleHelp.value(entry->text()).contains(qsl("helpURL"))
                                || mpHost->moduleHelp.value(entry->text()).value(qsl("helpURL")).isEmpty()));
    } else {
        helpButton->setDisabled(true);
    }
}

void dlgModuleManager::slot_moduleChanged(QTableWidgetItem* pItem)
{
    if (!mpHost) {
        return;
    }

    const int i = pItem->row();

    QStringList moduleStringList;
    QTableWidgetItem* entry = moduleTable->item(i, 0);
    QTableWidgetItem* checkStatus = moduleTable->item(i, 2);
    QTableWidgetItem* itemPriority = moduleTable->item(i, 1);
    if (!entry || !checkStatus || !itemPriority || !mpHost->mInstalledModules.contains(entry->text())) {
        return;
    }
    moduleStringList = mpHost->mInstalledModules.value(entry->text());
    if (checkStatus->checkState() == Qt::Checked) {
        moduleStringList[1] = QLatin1String("1");
    } else {
        moduleStringList[1] = QLatin1String("0");
    }
    mpHost->mInstalledModules[entry->text()] = moduleStringList;
    mpHost->mModulePriorities[entry->text()] = itemPriority->text().toInt();
}

void dlgModuleManager::slot_helpModule()
{
    if (!mpHost) {
        return;
    }
    const int cRow = moduleTable->currentRow();
    QTableWidgetItem* pI = moduleTable->item(cRow, 0);
    if (!pI) {
        return;
    }
    if (mpHost->moduleHelp.value(pI->text()).contains(QLatin1String("helpURL")) && !mpHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")).isEmpty()) {
        if (!mudlet::self()->openWebPage(mpHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")))) {
            //failed first open, try for a module related path
            QTableWidgetItem* item = moduleTable->item(cRow, 3);
            const QString itemPath = item->text();
            QStringList path = itemPath.split(QDir::separator());
            path.pop_back();
            path.append(QDir::separator());
            path.append(mpHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")));
            const QString path2 = path.join(QString());
            if (!mudlet::self()->openWebPage(path2)) {
                helpButton->setDisabled(true);
            }
        }
    }
}
