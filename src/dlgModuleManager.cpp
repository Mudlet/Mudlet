/***************************************************************************
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2011 by Chris Mitchell                                  *
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
#include "ui_module_manager.h"
#include "mudlet.h"

#include <QFileDialog>
#include <QMessageBox>


dlgModuleManager::dlgModuleManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::module_manager)
, mpHost(pHost)
{
    ui->setupUi(this);
    mModuleTable = ui->moduleTable;
    mModuleUninstallButton = ui->uninstallButton;
    mModuleInstallButton = ui->installButton;
    mModuleHelpButton = ui->helpButton;

    layoutModules();
    connect(mModuleUninstallButton, &QAbstractButton::clicked, this, &dlgModuleManager::slot_uninstall_module);
    connect(mModuleInstallButton, &QAbstractButton::clicked, this, &dlgModuleManager::slot_install_module);
    connect(mModuleHelpButton, &QAbstractButton::clicked, this, &dlgModuleManager::slot_help_module);
    connect(mModuleTable, &QTableWidget::itemClicked, this, &dlgModuleManager::slot_module_clicked);
    connect(mModuleTable, &QTableWidget::itemChanged, this, &dlgModuleManager::slot_module_changed);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgModuleManager::close);
    setWindowTitle(tr("Module Manager - %1").arg(mpHost->getName()));
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgModuleManager::~dlgModuleManager()
{
    mpHost->mpModuleManager = nullptr;
    delete ui;
}

void dlgModuleManager::layoutModules()
{
    if (!mpHost) {
        return;
    }

    QMapIterator<QString, QStringList> it(mpHost->mInstalledModules);
    QStringList sl;
    sl << tr("Module Name") << tr("Priority") << tr("Sync") << tr("Module Location");
    mModuleTable->setHorizontalHeaderLabels(sl);
    mModuleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mModuleTable->verticalHeader()->hide();
    mModuleTable->setShowGrid(true);
    //clear everything
    for (int i = 0; i <= mModuleTable->rowCount(); i++) {
        mModuleTable->removeRow(i);
    }
    //order modules by priority and then alphabetically
    QMap<int, QStringList> mOrder;
    while (it.hasNext()) {
        it.next();
        int priority = mpHost->mModulePriorities[it.key()];
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
            int row = mModuleTable->rowCount();
            mModuleTable->insertRow(row);
            auto masterModule = new QTableWidgetItem();
            auto itemEntry = new QTableWidgetItem();
            auto itemLocation = new QTableWidgetItem();
            auto itemPriority = new QTableWidgetItem();
            QStringList moduleInfo = mpHost->mInstalledModules[pModules[i]];
            QFileInfo moduleFile = moduleInfo[0];
            QStringList accepted_suffix;
            accepted_suffix << "xml" << "trigger";
            if (accepted_suffix.contains(moduleFile.suffix().trimmed(), Qt::CaseInsensitive)) {
                masterModule->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                masterModule->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                                 .arg(tr("Checking this box will cause the module to be saved and <i>resynchronised</i> across all "
                                                         "sessions that share it when the <i>Save Profile</i> button is clicked in the Editor or if it "
                                                         "is saved at the end of the session.")));
            } else {
                masterModule->setFlags(Qt::NoItemFlags);
                masterModule->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                                 .arg(tr("<b>Note:</b> <i>.zip</i> and <i>.mpackage</i> modules are currently unable to be synced<br> "
                                                         "only <i>.xml</i> packages are able to be synchronized across profiles at the moment. ")));
            }


            if (moduleInfo.at(1).toInt()) {
                masterModule->setCheckState(Qt::Checked);
            } else {
                masterModule->setCheckState(Qt::Unchecked);
            }
            masterModule->setText(QString());

            // Although there is now no text used here this may help to make the
            // checkbox more central in the column
            masterModule->setTextAlignment(Qt::AlignCenter);

            QString moduleName = pModules[i];
            itemEntry->setText(moduleName);
            itemEntry->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            itemLocation->setText(moduleInfo[0]);
            itemLocation->setToolTip(moduleInfo[0]);                          // show the full path in a tooltip, in case it doesn't fit in the table
            itemLocation->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); // disallow editing of module path, because that is not saved
            itemPriority->setData(Qt::EditRole, mpHost->mModulePriorities[moduleName]);
            mModuleTable->setItem(row, 0, itemEntry);
            mModuleTable->setItem(row, 1, itemPriority);
            mModuleTable->setItem(row, 2, masterModule);
            mModuleTable->setItem(row, 3, itemLocation);
        }
    }
    mModuleTable->resizeColumnsToContents();
}

void dlgModuleManager::slot_install_module()
{
    if (!mpHost) {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Mudlet Module"), QDir::currentPath());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Load Mudlet Module:"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    mpHost->installPackage(fileName, 1);
    for (int i = mModuleTable->rowCount() - 1; i >= 0; --i) {
        mModuleTable->removeRow(i);
    }

    layoutModules();
}

void dlgModuleManager::slot_uninstall_module()
{
    if (!mpHost) {
        return;
    }

    int cRow = mModuleTable->currentRow();
    QTableWidgetItem* pI = mModuleTable->item(cRow, 0);
    if (pI) {
        mpHost->uninstallPackage(pI->text(), 1);
    }
    for (int i = mModuleTable->rowCount() - 1; i >= 0; --i) {
        mModuleTable->removeRow(i);
    }
    layoutModules();
}

void dlgModuleManager::slot_module_clicked(QTableWidgetItem* pItem)
{
    if (!mpHost) {
        return;
    }

    int i = pItem->row();

    QTableWidgetItem* entry = mModuleTable->item(i, 0);
    QTableWidgetItem* checkStatus = mModuleTable->item(i, 2);
    QTableWidgetItem* itemPriority = mModuleTable->item(i, 1);
    //  Not used programatically now: QTableWidgetItem* itemPath = moduleTable->item(i, 3);
    if (!entry || !checkStatus || !itemPriority || !mpHost->mInstalledModules.contains(entry->text())) {
        mModuleHelpButton->setDisabled(true);
        if (checkStatus) {
            checkStatus->setCheckState(Qt::Unchecked);
            checkStatus->setFlags(Qt::NoItemFlags);
        }
        return;
    }

    if (mpHost->moduleHelp.contains(entry->text())) {
        mModuleHelpButton->setDisabled((!mpHost->moduleHelp.value(entry->text()).contains(QStringLiteral("helpURL"))
                                       || mpHost->moduleHelp.value(entry->text()).value(QStringLiteral("helpURL")).isEmpty()));
    } else {
        mModuleHelpButton->setDisabled(true);
    }
}

void dlgModuleManager::slot_module_changed(QTableWidgetItem* pItem)
{
    if (!mpHost) {
        return;
    }

    int i = pItem->row();

    QStringList moduleStringList;
    QTableWidgetItem* entry = mModuleTable->item(i, 0);
    QTableWidgetItem* checkStatus = mModuleTable->item(i, 2);
    QTableWidgetItem* itemPriority = mModuleTable->item(i, 1);
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

void dlgModuleManager::slot_help_module()
{
    if (!mpHost) {
        return;
    }
    int cRow = mModuleTable->currentRow();
    QTableWidgetItem* pI = mModuleTable->item(cRow, 0);
    if (!pI) {
        return;
    }
    if (mpHost->moduleHelp.value(pI->text()).contains(QLatin1String("helpURL")) && !mpHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")).isEmpty()) {
        if (!mudlet::self()->openWebPage(mpHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")))) {
            //failed first open, try for a module related path
            QTableWidgetItem* item = mModuleTable->item(cRow, 3);
            QString itemPath = item->text();
            QStringList path = itemPath.split(QDir::separator());
            path.pop_back();
            path.append(QDir::separator());
            path.append(mpHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")));
            QString path2 = path.join(QString());
            if (!mudlet::self()->openWebPage(path2)) {
                mModuleHelpButton->setDisabled(true);
            }
        }
    }
}
