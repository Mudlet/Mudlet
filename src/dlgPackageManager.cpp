/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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


#include "dlgPackageManager.h"

#include "mudlet.h"

#include "pre_guard.h"
#include <QFileDialog>
#include <QScrollBar>
#include <QMessageBox>
#include "post_guard.h"


dlgPackageManager::dlgPackageManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, mpHost(pHost)
{
    setupUi(this);
    resetPackageTable();
    connect(packageTable, &QTableWidget::itemClicked, this, &dlgPackageManager::slot_itemClicked);
    connect(installButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_installPackage);
    connect(removeButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_removePackages);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgPackageManager::close);
    connect(packageTable, &QTableWidget::currentItemChanged, this, &dlgPackageManager::slot_itemClicked);
    connect(packageTable, &QTableWidget::itemSelectionChanged, this, &dlgPackageManager::slot_toggleRemoveButton);

    setWindowTitle(tr("Package Manager - %1").arg(mpHost->getName()));
    additionalDetails->setEditTriggers(QAbstractItemView::NoEditTriggers);
    additionalDetails->setFocusPolicy(Qt::NoFocus);
    additionalDetails->setSelectionMode(QAbstractItemView::NoSelection);
    packageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    packageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    packageTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    additionalDetails->hide();
    detailsLabel->hide();
    packageDescription->hide();
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgPackageManager::~dlgPackageManager()
{
}

void dlgPackageManager::resetPackageTable()
{
    if (!mpHost) {
        return;
    }
    for (int i =  packageTable->rowCount() - 1; i >= 0; --i) {
        packageTable->removeRow(i);
    }

    packageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
        packageTable->insertRow(i);
        auto packageName = new QTableWidgetItem();
        auto shortDescription = new QTableWidgetItem();
        packageName->setTextAlignment(Qt::AlignCenter);
        QFont nameFont;
        nameFont.setBold(true);
        packageName->setFont(nameFont);
        shortDescription->setTextAlignment(Qt::AlignCenter);
        packageName->setText(mpHost->mInstalledPackages.at(i));
        auto packageInfo{mpHost->mPackageInfo.value(packageName->text())};
        auto iconName = packageInfo.value(qsl("icon"));
        if (!iconName.isEmpty()) {
            auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(packageName->text(), iconName));
            packageName->setIcon(QIcon(iconDir));
        }
        auto title = packageInfo.value(qsl("title"));
        shortDescription->setText(title);
        packageTable->setItem(i, 0, packageName);
        packageTable->setItem(i, 1, shortDescription);
    }
    packageTable->resizeColumnsToContents();
}

void dlgPackageManager::slot_installPackage()
{
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), lastDir);
    if (fileName.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Import Mudlet Package:"), tr("Cannot read file %1:\n%2.").arg(fileName.toHtmlEscaped(), file.errorString()));
        return;
    }

    mpHost->installPackage(fileName, enums::PackageModuleType::Package);
}

void dlgPackageManager::slot_removePackages()
{
    const QModelIndexList selection = packageTable->selectionModel()->selectedRows();
    QStringList removePackages;
    for (const QModelIndex& index : selection) {
        auto package = packageTable->item(index.row(), 0);
        removePackages << package->text();
    }

    for (const QString& package : removePackages) {
        mpHost->uninstallPackage(package, enums::PackageModuleType::Package);
    }

    additionalDetails->hide();
    detailsLabel->hide();
    packageDescription->hide();
}

void dlgPackageManager::slot_itemClicked(QTableWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    //clear details Table
    for (int i = additionalDetails->rowCount() - 1; i >= 0; --i) {
        additionalDetails->removeRow(i);
    }
    const QString packageName = packageTable->item(pItem->row(), 0)->text();
    auto packageInfo{mpHost->mPackageInfo.value(packageName)};
    if (packageInfo.isEmpty()) {
        packageDescription->clear();
        additionalDetails->hide();
        detailsLabel->hide();
        packageDescription->hide();
        return;
    }
    packageInfo.remove(qsl("mpackage"));
    packageInfo.remove(qsl("icon"));
    packageInfo.remove(qsl("title"));

    QString description = packageInfo.take(qsl("description"));
    if (description.isEmpty()) {
        packageDescription->hide();
    } else {
        packageDescription->show();
        const QString packageDir = mudlet::self()->getMudletPath(enums::profileDataItemPath, mpHost->getName(), packageName);
        description.replace(QLatin1String("$packagePath"), packageDir);
        packageDescription->setMarkdown(description);
    }

    QStringList labelText, details;
    labelText << tr("Author") << tr("Version") << tr("Created") << tr("Dependencies");
    details << qsl("author") << qsl("version") << qsl("created") << qsl("dependencies");
    int counter = 0;
    for (int i = 0; i < details.size(); i++) {
        const QString valueText{packageInfo.take(details.at(i))};
        if (valueText.isEmpty()) {
            continue;
        }
        QLabel* info = new QLabel();
        QLabel* value = new QLabel();
        info->setEnabled(false);
        additionalDetails->insertRow(counter);
        additionalDetails->setCellWidget(counter, 0, info);
        additionalDetails->setCellWidget(counter++, 1, value);
        info->setText(labelText.at(i));
        info->setAlignment(Qt::AlignLeft);
        value->setText(valueText);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setAlignment(Qt::AlignLeft);
    }

    if (!packageInfo.isEmpty()) {
        fillAdditionalDetails(packageInfo);
    }
    additionalDetails->resizeColumnsToContents();
    additionalDetails->resizeRowsToContents();
    additionalDetails->horizontalHeader()->resizeSection(0, additionalDetails->horizontalHeader()->sectionSize(0) + 10);
    if (additionalDetails->rowCount() == 0) {
        additionalDetails->hide();
        detailsLabel->hide();
    } else {
        additionalDetails->show();
        detailsLabel->show();
    }
    const int maxHeight = additionalDetails->rowCount() * additionalDetails->rowHeight(0);
    additionalDetails->setMaximumHeight(maxHeight);
    additionalDetails->verticalScrollBar()->hide();
    packageTable->scrollToItem(pItem);
    packageTable->selectRow(pItem->row());
}

void dlgPackageManager::fillAdditionalDetails(const QMap<QString, QString>& packageInfo)
{
    QMap<QString, QString>::const_iterator iter = packageInfo.constBegin();
    int counter = additionalDetails->rowCount();
    while (iter != packageInfo.constEnd()) {
        QLabel* info = new QLabel();
        QLabel* value = new QLabel();
        info->setEnabled(false);
        additionalDetails->insertRow(counter);
        additionalDetails->setCellWidget(counter, 0, info);
        additionalDetails->setCellWidget(counter++, 1, value);
        info->setText(iter.key());
        info->setAlignment(Qt::AlignLeft);
        value->setText(iter.value());
        value->setOpenExternalLinks(true);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse);
        value->setAlignment(Qt::AlignLeft);
        ++iter;
    }
}

void dlgPackageManager::slot_toggleRemoveButton()
{
    const QModelIndexList selection = packageTable->selectionModel()->selectedRows();
    const int selectionCount = selection.count();
    removeButton->setEnabled(selectionCount);
    if (selectionCount) {
        //: Message on button in package manager to remove one or more (%n is the count of) selected package(s).
        removeButton->setText(tr("Remove %n package(s)", nullptr, selectionCount));
    } else {
        //: Message on button in package manager initially and when there is no packages to remove
        removeButton->setText(tr("Remove package"));
    }
}
